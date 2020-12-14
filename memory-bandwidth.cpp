#include "cpu-feature.h"
#include "memory-bandwidth.h"
#include "ipc.h"
#include "memalloc.h"
#include "oneshot_timer.h"
#include "sys-microbenchmark.h"
#include "table.h"
#include "thread.h"
#include "x86funcs.h"
#include <atomic>

namespace smbm {

namespace {

struct CopyTest {
    const char *name;
    void (*op)(void *dst, void const *src, size_t sz);
    bool (*supported)();
};
struct StoreTest {
    const char *name;
    void (*op)(void *dst, size_t sz);
    bool (*supported)();
};
struct LoadTest {
    const char *name;
    uint64_t (*op)(void const *src, size_t sz);
    bool (*supported)();
};

static void memcpy_test(void *dst, void const *src, size_t sz) {
    memcpy(dst, src, sz);
}
static void memset_test(void *dst, size_t sz) { memset(dst, 0, sz); }

static void simple_long_copy_test(void *dst, void const *src, size_t sz) {
    size_t nloop = sz / 8;
    const uint64_t *ps = (const uint64_t *)src;
    uint64_t *pd = (uint64_t *)dst;

    for (size_t i = 0; i < nloop; i++) {
        pd[i] = ps[i];
    }
}
static void simple_long_store_test(void *dst, size_t sz) {
    size_t nloop = sz / 8;
    uint64_t *pd = (uint64_t *)dst;

    for (size_t i = 0; i < nloop; i++) {
        pd[i] = 0;
    }
}

static bool T() { return true; }

static void gccvec128_copy_test(void *dst, void const *src, size_t sz) {
    size_t nloop = sz / 16;
    const vec128i *ps = (const vec128i *)src;
    vec128i *pd = (vec128i *)dst;

    for (size_t i = 0; i < nloop; i += 4) {
        vec128i v0 = ps[i + 0];
        vec128i v1 = ps[i + 1];
        vec128i v2 = ps[i + 2];
        vec128i v3 = ps[i + 3];

        pd[i + 0] = v0;
        pd[i + 1] = v1;
        pd[i + 2] = v2;
        pd[i + 3] = v3;
    }
}
static void gccvec128_store_test(void *dst, size_t sz) {
    size_t nloop = sz / 16;
    vec128i *pd = (vec128i *)dst;

    vec128i zero = {0, 0};

    for (size_t i = 0; i < nloop; i++) {
        pd[i] = zero;
    }
}

inline void wait_barrier(std::atomic<int> *p, int wait_count) {
    wmb();
    (*p) += 1;

    while (1) {
        if ((*p) == wait_count) {
            break;
        }
    }
    rmb();
}

enum class memop { COPY, STORE, LOAD, QUIT };

union fn_union {
    void (*p_copy_fn)(void *dst, void const *src, size_t sz);
    void (*p_store_fn)(void *dst, size_t sz);
    uint64_t (*p_load_fn)(void const *src, size_t sz);
};

struct ThreadInfo {
    thread_handle_t t;
    int self_cpu;
    int total_thread_num;

    Pipe notify_to_copy_thread;
    Pipe notify_from_copy_thread;

    double duration_sec;
    double actual_sec;
    size_t buffer_size;
    const GlobalState *g;

    memop com;
    fn_union fn;
    std::atomic<int> *start_barrier;
    std::atomic<int> *end_barrier;

    size_t total_transfer;
};

static inline void invoke_memory_func(ThreadInfo *ti, void *dst, void *src) {
    uint64_t r;
    switch (ti->com) {
    case memop::COPY:
        ti->fn.p_copy_fn(dst, src, ti->buffer_size);
        break;

    case memop::STORE:
        ti->fn.p_store_fn(dst, ti->buffer_size);
        break;

    case memop::LOAD:
        r = ti->fn.p_load_fn(src, ti->buffer_size);
        ti->g->dummy_write(ti->self_cpu, r);
        break;

    case memop::QUIT:
        abort();
    }
}

static void *mem_thread(void *p) {
    ThreadInfo *ti = (ThreadInfo *)p;

    {
        ProcessorIndex idx = ti->g->proc_table->logical_index_to_processor(ti->self_cpu, PROC_ORDER_OUTER_TO_INNER);
        bind_self_to_1proc(ti->g->proc_table,
                           idx,
                           true);
    }

    void *src = aligned_calloc(64, ti->buffer_size);
    void *dst = aligned_calloc(64, ti->buffer_size);

    warmup_thread(ti->g);

    while (1) {
        read_pipe(&ti->notify_to_copy_thread);

        if (ti->com == memop::QUIT) {
            break;
        }

        invoke_memory_func(ti, dst, src);

        wait_barrier(ti->start_barrier, ti->total_thread_num);

        auto t0 = userland_timer_value::get();

        oneshot_timer ot(16);
        uint64_t call_count = 0;

        ot.start(ti->g, ti->duration_sec);

        while (!ot.test_end()) {
            invoke_memory_func(ti, dst, src);

            call_count++;
        }

        wait_barrier(ti->end_barrier, ti->total_thread_num);
        auto t1 = userland_timer_value::get();

        ti->actual_sec = ti->g->userland_timer_delta_to_sec(t1 - t0);
        ti->total_transfer = call_count * ti->buffer_size;

        write_pipe(&ti->notify_from_copy_thread, 0);
    }

    aligned_free(src);
    aligned_free(dst);

    return nullptr;
}

static const CopyTest copy_tests[] = {
    {"simple-long-copy", simple_long_copy_test, T},
    {"libc-memcpy", memcpy_test, T},
    {"gccvec128-copy", gccvec128_copy_test, T},

#ifdef X86
    {"sse-stream-copy", sse_stream_copy, T},
    {"avx256-copy", avx256_copy, have_avx},
    {"avx512-copy", avx512_copy, have_avx512f},

    {"x86-rep-movs1", x86_rep_movs1, T},
    {"x86-rep-movs2", x86_rep_movs2, T},
    {"x86-rep-movs4", x86_rep_movs4, T},
#endif
};
static const StoreTest store_tests[] = {
    {"libc-memset", memset_test, T},
    {"simple-long-store", simple_long_store_test, T},
    {"gccvec128-store", gccvec128_store_test, T},

#ifdef X86
    {"sse-stream-store", sse_stream_store, T},
    {"avx256-store", avx256_store, have_avx},
    {"avx256-stream-store", avx256_stream_store, have_avx},
    {"avx512-store", avx512_store, have_avx512f},
    {"avx512-stream-store", avx512_stream_store, have_avx512f},

    {"x86-rep-stos1", x86_rep_stos1, T},
    {"x86-rep-stos2", x86_rep_stos2, T},
    {"x86-rep-stos4", x86_rep_stos4, T},
#endif

};
static const LoadTest load_tests[] = {
    {"simple-long-sum", simple_long_sum_test, T},
    {"gccvec128-load", gccvec128_load_test, T},

#ifdef X86
    {"avx256-load", avx256_load, have_avx},
    {"avx512-load", avx512_load, have_avx512f},

    {"x86-rep-scas1", x86_rep_scas1, T},
    {"x86-rep-scas2", x86_rep_scas2, T},
    {"x86-rep-scas4", x86_rep_scas4, T},
#endif

};

static double run1(ThreadInfo *t, int num_thread, memop op, fn_union u) {
    *(t[0].start_barrier) = 0;
    *(t[0].end_barrier) = 0;

    for (int i = 0; i < num_thread; i++) {
        t[i].com = op;
        t[i].fn = u;
        write_pipe(&t[i].notify_to_copy_thread, 0);
    }

    size_t total_transfer = 0;
    for (int i = 0; i < num_thread; i++) {
        read_pipe(&t[i].notify_from_copy_thread);
        total_transfer += t[i].total_transfer;
    }

    double sec = t[0].actual_sec;
    double bytes_per_sec = total_transfer / sec;

    return bytes_per_sec;
}

static ThreadInfo *init_threads(const GlobalState *g, int start_proc, int num_thread,
                                double duration_sec, size_t buffer_size) {
    ThreadInfo *t = new ThreadInfo[num_thread];

    std::atomic<int> *start_barrier = new std::atomic<int>;
    std::atomic<int> *end_barrier = new std::atomic<int>;

    for (int i = 0; i < num_thread; i++) {
        t[i].start_barrier = start_barrier;
        t[i].end_barrier = end_barrier;
        t[i].total_thread_num = num_thread;
        t[i].duration_sec = duration_sec;
        t[i].buffer_size = buffer_size;
        t[i].g = g;
        int run_on = start_proc + i;
        t[i].self_cpu = run_on;

        t[i].t = spawn_thread(mem_thread, &t[i]);
    }

    return t;
}

static void finish_threads(ThreadInfo *t, int num_thread) {
    delete t[0].start_barrier;
    delete t[0].end_barrier;

    for (int i = 0; i < num_thread; i++) {
        t[i].com = memop::QUIT;
        write_pipe(&t[i].notify_to_copy_thread, 0);
        wait_thread(t[i].t);
    }

    delete[] t;
}

} // namespace

uint64_t gccvec128_load_test(void const *src, size_t sz) {
    size_t nloop = sz / 16;
    const vec128i *p = (const vec128i *)src;

    vec128i sum0 = {0, 0};
    vec128i sum1 = {0, 0};
    vec128i sum2 = {0, 0};
    vec128i sum3 = {0, 0};

    for (size_t i = 0; i < nloop; i += 4) {
        sum0 += p[i + 0];
        sum1 += p[i + 1];
        sum2 += p[i + 2];
        sum3 += p[i + 3];
    }

    sum0 = sum0 + sum1;
    sum2 = sum2 + sum3;

    sum0 = sum0 + sum2;

    return sum0[0] + sum0[1];
}

uint64_t simple_long_sum_test(void const *src, size_t sz) {
    size_t nloop = sz / 8;
    const uint64_t *p = (const uint64_t *)src;
    uint64_t sum = 0;

    for (size_t i = 0; i < nloop; i++) {
        sum += p[i];
    }
    return sum;
}


#if 0
static void do_test(struct thread_shared *clients, char *dst, char *src,
                    size_t max_size, int nproc, opfn_t opfn,
                    const char *test_name, int mul) {
    int ni = 1;
    int last = 0;

    while (!last) {
        size_t copy_size = 1024;

        if (ni >= nproc) {
            ni = nproc;
            last = 1;
        }

        printf("num_thread = %d\n", ni);

        while (copy_size <= max_size) {
            int niter = 16384;

            niter /= (copy_size / 512);
            if (niter < 4) {
                niter = 4;
            }

            double t0 = sec();
            for (int ti = 0; ti < ni - 1; ti++) {
                clients[ti].op_fn = opfn;
                clients[ti].num_iter = niter;
                clients[ti].copy_size = copy_size;

                cpu_wmb();

                clients[ti].start = 1;
            }

            run_test1(dst, src, opfn, niter, copy_size);

            for (int ti = 0; ti < ni - 1; ti++) {
                while (1) {
                    if (clients[ti].end == 1) {
                        clients[ti].end = 0;
                        break;
                    }

                    compiler_mb();
                }
            }
            double t1 = sec();

            size_t transfer_size = copy_size * mul;
            double total = ni * transfer_size * niter;
            double bps = total / (t1 - t0);

            if (transfer_size < 16 * 1024) {
                printf("%-16s : %8d[ B] %f[GB/s]\n", test_name,
                       (int)transfer_size, bps / (1024 * 1024 * 1024.0));
            } else if (transfer_size < 16 * 1024ULL * 1024ULL) {
                printf("%-16s : %8d[KB] %f[GB/s]\n", test_name,
                       (int)transfer_size / 1024, bps / (1024 * 1024 * 1024.0));
            } else {
                printf("%-16s : %8d[MB] %f[GB/s]\n", test_name,
                       (int)transfer_size / (1024 * 1024),
                       bps / (1024 * 1024 * 1024.0));
            }
            copy_size *= 2;
        }
        ni *= 2;
    }
}
#endif

struct MemoryBandwidth : public BenchDesc {
    typedef Table1D<double, std::string> table_t;
    bool full_thread;

    MemoryBandwidth(bool full_thread)
        : BenchDesc(full_thread ? "membw_mt" : "membw_1t"),
          full_thread(full_thread) {}

    virtual result_t run(const GlobalState *g) override {
        int ncopy_test = sizeof(copy_tests) / sizeof(copy_tests[0]);
        int nload_test = sizeof(load_tests) / sizeof(load_tests[0]);
        int nstore_test = sizeof(store_tests) / sizeof(store_tests[0]);

        int ntest = 0;
        std::vector<std::string> test_name_list;

        for (int i = 0; i < ncopy_test; i++) {
            if (copy_tests[i].supported()) {
                test_name_list.push_back(copy_tests[i].name);
                ntest++;
            }
        }
        for (int i = 0; i < nload_test; i++) {
            if (load_tests[i].supported()) {
                test_name_list.push_back(load_tests[i].name);
                ntest++;
            }
        }
        for (int i = 0; i < nstore_test; i++) {
            if (store_tests[i].supported()) {
                test_name_list.push_back(store_tests[i].name);
                ntest++;
            }
        }

        table_t *result = new table_t("test_name", ntest);
        result->column_label = "MiB/sec";
        result->row_label = test_name_list;

        int max_thread = g->proc_table->get_active_cpu_count();

        {
            // move to unused core
            ProcessorIndex idx = g->proc_table->logical_index_to_processor(max_thread-1, PROC_ORDER_OUTER_TO_INNER);
            bind_self_to_1proc(g->proc_table,
                               idx,
                               true);
        }


        int start_proc = 0;
        int nthread = 1;
        if (this->full_thread) {
            if (max_thread >= 8) {
                start_proc = 0; // skip first proc
                nthread = max_thread / 2;
            } else {
                nthread = max_thread;
            }
        }

        size_t test_size = 128 * 1024 * 1024 / nthread;

        ThreadInfo *threads = init_threads(g, start_proc, nthread, 0.1, test_size);

        union fn_union fn;
        int cur = 0;

        for (int i = 0; i < ncopy_test; i++) {
            if (copy_tests[i].supported()) {
                fn.p_copy_fn = copy_tests[i].op;
                double bps = run1(threads, nthread, memop::COPY, fn);
                result->v[cur] = (bps * 2) / (1024.0 * 1024.0); // MiB
                cur++;
            }
        }

        for (int i = 0; i < nload_test; i++) {
            if (load_tests[i].supported()) {
                fn.p_load_fn = load_tests[i].op;
                double bps = run1(threads, nthread, memop::LOAD, fn);
                result->v[cur] = bps / (1024.0 * 1024.0); // MiB
                cur++;
            }
        }

        for (int i = 0; i < nstore_test; i++) {
            if (store_tests[i].supported()) {
                fn.p_store_fn = store_tests[i].op;
                double bps = run1(threads, nthread, memop::STORE, fn);
                result->v[cur] = bps / (1024.0 * 1024.0); // MiB
                cur++;
            }
        }

        finish_threads(threads, nthread);

        {
            // reset
            ProcessorIndex idx = g->proc_table->logical_index_to_processor(0, PROC_ORDER_OUTER_TO_INNER);
            bind_self_to_1proc(g->proc_table,
                               idx,
                               true);
        }


        return result_t(result);
    }

    virtual result_t parse_json_result(picojson::value const &v) {
        return result_t(table_t::parse_json_result(v));
    }
};

struct CacheBandwidth : public BenchDesc {
    typedef Table2D<double, std::string, int> table_t; // row=bytes, column=thread

    CacheBandwidth()
        :BenchDesc("cache-bandwidth")
    {}

    virtual result_t run(const GlobalState *g) override {
        return result_t(new table_t("amount", "thread num", 4, 4));
    }


    virtual result_t parse_json_result(picojson::value const &v) {
        return result_t(table_t::parse_json_result(v));
    }
};


std::unique_ptr<BenchDesc> get_memory_bandwidth_1thread_desc() {
    return std::unique_ptr<BenchDesc>(new MemoryBandwidth(false));
}
std::unique_ptr<BenchDesc> get_memory_bandwidth_full_thread_desc() {
    return std::unique_ptr<BenchDesc>(new MemoryBandwidth(true));
}

std::unique_ptr<BenchDesc> get_cache_bandwidth_desc() {
    return std::unique_ptr<BenchDesc>(new CacheBandwidth());

}

} // namespace smbm
