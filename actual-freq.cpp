#include "actual-freq.h"
#include "barrier.h"
#include "cpu-feature.h"
#include "cpuset.h"
#include "oneshot_timer.h"
#include "sys-microbenchmark.h"
#include "table.h"
#include "thread.h"
#include "x86funcs.h"

namespace smbm {

namespace {

using namespace af;

#ifdef HAVE_HW_PERF_COUNTER

struct ThreadInfo {
    thread_handle_t thread;

    uint64_t (*func)(uint64_t zero, oneshot_timer *ot);

    GlobalState const *g;
    int tid;
    atomic_int_t *start_barrier;
    int total_thread_num;
    double delay;

    /* result */
    double realtime_sec;
    uint64_t cycle_delta;
};

static void *thread_func(void *ap) {
    ThreadInfo *ti = (ThreadInfo *)ap;

    ProcessorIndex idx = ti->g->proc_table->logical_index_to_processor(
        ti->tid, PROC_ORDER_OUTER_TO_INNER);
    bind_self_to_1proc(ti->g->proc_table, idx, true);

    perf_counter_value_t pt0 = 0, pt1;
    oneshot_timer ot(128);

    uint64_t zero = ti->g->getzero();

    wait_barrier(ti->start_barrier, ti->total_thread_num);
    ot.start(ti->g, ti->delay);

    if (ti->tid == 0) {
        pt0 = ti->g->get_hw_cpucycle();
    }

    uint64_t x = ti->func(zero, &ot);

    if (ti->tid == 0) {
        pt1 = ti->g->get_hw_cpucycle();

        ti->cycle_delta = pt1 - pt0;
        ti->realtime_sec = ot.actual_interval_sec(ti->g);
    }

    ti->g->dummy_write(ti->tid, x);

    return 0;
}

static uint64_t iadd64(uint64_t zero, oneshot_timer *ot) {
    return op(zero, zero, zero, ot, [](auto x, auto y) { return x + y; });
}
static uint64_t fadd64(uint64_t zero, oneshot_timer *ot) {
    double x = zero;
    return opf(x, x, x, ot, [](auto x, auto y) { return x + y; });
}
static uint64_t iadd32x4(uint64_t zero, oneshot_timer *ot) {
    typedef uint32_t vec128i __attribute__((vector_size(16)));

    vec128i x = {(uint32_t)zero};
    vec128i ret = op(x, x, x, ot, [](auto x, auto y) { return x + y; });
    return ret[0] + ret[1] + ret[2] + ret[3];
}
static uint64_t fadd64x2(uint64_t zero, oneshot_timer *ot) {
    typedef double vec128d __attribute__((vector_size(16)));

    vec128d x = {(double)zero};
    vec128d ret = opf(x, x, x, ot, [](auto x, auto y) { return x + y; });

    return ret[0] + ret[1];
}

static uint64_t fmul64x2(uint64_t zero, oneshot_timer *ot) {
    typedef double vec128d __attribute__((vector_size(16)));

    vec128d x = {(double)zero};
    vec128d ret = opf(x, x, x, ot, [](auto x, auto y) { return x * y; });

    return ret[0] + ret[1];
}

static double run1(ThreadInfo *ti, int nthread,
                   uint64_t (*func)(uint64_t zero, oneshot_timer *ot)) {
    (*ti[0].start_barrier) = 0;

    for (int i = 0; i < nthread; i++) {
        ti[i].func = func;
        ti[i].total_thread_num = nthread;

        if (i != 0) {
            ti[i].thread = spawn_thread(thread_func, &ti[i]);
        }
    }

    thread_func(&ti[0]);

    for (int i = 1; i < nthread; i++) {
        wait_thread(ti[i].thread);
    }

    double real_sec = ti[0].realtime_sec;
    uint64_t cycle = ti[0].cycle_delta;

    double real_freq = cycle / real_sec;

    return real_freq;
}

static constexpr bool T() { return true; }

#define FOR_EACH_INST_GENERIC(F)                                               \
    F(iadd64, T)                                                               \
    F(fadd64, T)                                                               \
    F(iadd32x4, T)                                                             \
    F(fadd64x2, T)                                                             \
    F(fmul64x2, T)

#ifdef X86

#define FOR_EACH_INST(F)                                                       \
    FOR_EACH_INST_GENERIC(F)                                                   \
    F(busy_iadd32x8, have_avx2)                                                \
    F(busy_imul32x8, have_avx2)                                                \
    F(busy_fadd64x4, have_avx)                                                 \
    F(busy_fmul64x4, have_avx)                                                 \
    F(busy_fma64x4, have_fma)                                                  \
    F(busy_iadd32x16, have_avx512f)                                            \
    F(busy_imul32x16, have_avx512f)                                            \
    F(busy_fadd64x8, have_avx512f)                                             \
    F(busy_fmul64x8, have_avx512f)                                             \
    F(busy_fma64x8, have_avx512f)

#else
#define FOR_EACH_INST(F) FOR_EACH_INST_GENERIC(F)
#endif

#endif

typedef Table2DBenchDesc<double, std::string, uint32_t> parent_t;

struct ActualFreq : public parent_t {
    ActualFreq() : parent_t("actual-freq", HIGHER_IS_BETTER) {}

    result_t run(GlobalState const *g) override {
#ifdef HAVE_HW_PERF_COUNTER
        std::vector<std::string> row_label;
        std::vector<uint32_t> threads;
        std::vector<double> delays = {0.01, 0.1, 0.2, 0.4};

        int max_thread = g->proc_table->get_active_cpu_count();
        int use_thread = max_thread - 1;

        for (int i = 1; i < max_thread; i *= 2) {
            threads.push_back(i);
        }
        threads.push_back(use_thread);

        for (auto delay : delays) {
            char buffer[128];

#define ADD_LABEL(name, cond)                                                  \
    if (cond()) {                                                              \
        sprintf(buffer, "%s (%.2f [sec])", #name, delay);                      \
        row_label.push_back(buffer);                                           \
    }

            FOR_EACH_INST(ADD_LABEL);
        }

        table_t *result_table(new table_t("instruction", "num_thread",
                                          row_label.size(), threads.size()));
        result_table->row_label = std::move(row_label);
        result_table->column_label = threads;

        atomic_int_t start_barrier;
        std::vector<ThreadInfo> thread_info_list(use_thread);

        for (int i = 0; i < use_thread; i++) {
            thread_info_list[i].g = g;
            thread_info_list[i].tid = i;
            thread_info_list[i].start_barrier = &start_barrier;
            thread_info_list[i].delay = 0.25;
        }

        for (int ti = 0; ti < (int)threads.size(); ti++) {
            int tc = threads[ti];
            double r;

            int row = 0;
            for (auto delay : delays) {
                for (int i = 0; i < use_thread; i++) {
                    thread_info_list[i].delay = delay;
                }

#define DO_RUN(name, cond)                                                     \
    if (cond()) {                                                              \
        r = run1(&thread_info_list[0], tc, name);                              \
        (*result_table)[row++][ti] = r / 1e9;                                  \
    }
                FOR_EACH_INST(DO_RUN);
            }
        }

        return std::unique_ptr<BenchResult>(result_table);
#else
        return result_t();
#endif
    }
    result_t parse_json_result(picojson::value const &v) override {
        return result_t(table_t::parse_json_result(v));
    }

    int double_precision() override { return 2; }

    bool available(const GlobalState *g) override {
#ifdef HAVE_HW_PERF_COUNTER
        return g->is_hw_perf_counter_available();
#else
        return false;
#endif
    }
};

} // namespace

std::unique_ptr<BenchDesc> get_actual_freq_desc() {
    return std::unique_ptr<BenchDesc>(new ActualFreq());
}

} // namespace smbm
