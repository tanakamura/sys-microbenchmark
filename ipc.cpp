#include "cpu-feature.h"
#include "oneshot_timer.h"
#include "sys-microbenchmark.h"
#include "table.h"
#include "thread.h"

namespace smbm {
namespace {

struct __attribute__((aligned(CACHELINE_SIZE))) Shared {
    int to_p0;
    int to_p1;
    atomic_int_t barrier;
};

struct ThreadInfo {
    thread_handle_t t;

    GlobalState const *g;
    double delay;
    int tid;
    int cpu;
    bool use_yield;

    Shared *s;

    double actual_sec;
    uint64_t roundtrip_count;
};

static void *thread_func(void *p) {
    ThreadInfo *ti = (ThreadInfo *)p;
    Shared *s = ti->s;

    if (ti->cpu != 0) {
        auto pi = ti->g->proc_table->logical_index_to_processor(
            ti->cpu, PROC_ORDER_INNER_TO_OUTER);
        bind_self_to_1proc(ti->g->proc_table, pi, true);
    }

    bool use_yield = ti->use_yield;

    wait_barrier(&ti->s->barrier, 2);

    if (ti->tid == 0) {
        int self_cur = 1;
        int another_cur = 0;

        oneshot_timer ot(1024);
        ot.start(ti->g, ti->delay);
        uint64_t count = 0;

        while (!ot.test_end()) {
            s->to_p1 = self_cur;
            wmb();
            self_cur = !self_cur;

            while (1) {
                if (s->to_p0 != another_cur) {
                    another_cur = !another_cur;
                    break;
                }
                compiler_mb();
                if (use_yield) {
                    yield_thread();
                }
            }

            count++;
        }

        s->to_p1 = 2;

        ti->actual_sec = ot.actual_interval_sec(ti->g);
        ti->roundtrip_count = count;
    } else {
        int self_cur = 1;
        int another_cur = 0;

        while (1) {
            while (1) {
                if (s->to_p1 == 2) {
                    goto end;
                }
                if (s->to_p1 != another_cur) {
                    another_cur = !another_cur;
                    break;
                }
                compiler_mb();
                if (use_yield) {
                    yield_thread();
                }
            }

            s->to_p0 = self_cur;
            wmb();
            self_cur = !self_cur;
        }
    end:;
    }

    return 0;
}

static void run1(ThreadInfo *ti, int cpu) {
    if (cpu == 0) {
        thread_func(ti);
    } else {
        ti->t = spawn_thread(thread_func, ti);
    }
}

static void wait1(ThreadInfo *ti, int cpu) {
    if (cpu != 0) {
        wait_thread(ti->t);
    }
}

typedef Table2DBenchDesc<double, uint32_t, uint32_t> parent_t;
struct IPC : public parent_t {
    bool use_yield;

    IPC(bool use_yield)
        : parent_t(use_yield ? "inter-processor-communication-with-yield"
                             : "inter-processor-communication"),
          use_yield(use_yield) {}

    virtual result_t run(GlobalState const *g) override {
        int max_thread = g->proc_table->get_active_cpu_count();

        std::vector<uint32_t> threads;
        for (int i = 0; i < max_thread; i++) {
            threads.push_back(i);
        }

        ThreadInfo ti[2];
        table_t *result_table(new table_t("thread id", "thread id",
                                          threads.size(), threads.size()));

        Shared s;

        result_table->row_label = threads;
        result_table->column_label = threads;

        ti[0].g = g;
        ti[0].tid = 0;
        ti[0].delay = 0.15;
        ti[0].s = &s;
        ti[0].use_yield = use_yield;

        ti[1].g = g;
        ti[1].tid = 1;
        ti[1].delay = 0.15;
        ti[1].s = &s;
        ti[1].use_yield = use_yield;

        for (int t0 = 0; t0 < max_thread; t0++) {
            for (int t1 = 0; t1 < max_thread; t1++) {
                if (t1 == t0) {
                    (*result_table)[t0][t1] = 0;
                    continue;
                }
                if (t0 > t1) {
                    (*result_table)[t0][t1] = (*result_table)[t1][t0];
                    continue;
                }

                s.to_p0 = 0;
                s.to_p1 = 0;
                s.barrier = 0;

                ti[0].cpu = t0;
                ti[1].cpu = t1;

                if (t0 == 0) {
                    run1(&ti[1], t1);
                    run1(&ti[0], t0);
                } else {
                    run1(&ti[0], t0);
                    run1(&ti[1], t1);
                }

                wait1(&ti[0], t0);
                wait1(&ti[1], t1);

                double ns_per_roundtrip =
                    (ti[0].actual_sec / ti[0].roundtrip_count) * 1e9;

                (*result_table)[t0][t1] = ns_per_roundtrip;
            }
        }

        return result_t(result_table);
    }

    bool available(const GlobalState *g) override {
        int max_thread = g->proc_table->get_active_cpu_count();
        if (max_thread == 1) {
            return false;
        }

#ifdef HAVE_YIELD_INSTRCUTION
        return true;
#else
        return !this->use_yield;
#endif
    }

    int double_precision() override { return 1; }
};

}; // namespace

std::unique_ptr<BenchDesc> get_inter_processor_communication_desc() {
    return std::unique_ptr<BenchDesc>(new IPC(false));
}
std::unique_ptr<BenchDesc> get_inter_processor_communication_yield_desc() {
    return std::unique_ptr<BenchDesc>(new IPC(true));
}

} // namespace smbm
