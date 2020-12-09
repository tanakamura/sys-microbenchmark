#include "sys-microbenchmark.h"
#include "table.h"
#include "cpu-feature.h"
#include "x86funcs.h"

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
static void memset_test(void *dst, size_t sz) {
    memset(dst, 0, sz);
}

static uint64_t simple_long_sum_test(void const  *src, size_t sz) {
    size_t nloop = sz/8;
    const uint64_t *p = (const uint64_t*)src;
    uint64_t sum = 0;

    for (size_t i=0;i<nloop; i++) {
        sum += p[i];
    }
    return sum;
}
static void simple_long_copy_test(void *dst, void const  *src, size_t sz) {
    size_t nloop = sz/8;
    const uint64_t *ps = (const uint64_t*)src;
    uint64_t *pd = (uint64_t*)dst;

    for (size_t i=0;i<nloop; i++) {
        pd[i] = ps[i];
    }
}
static void simple_long_store_test(void *dst, size_t sz) {
    size_t nloop = sz/8;
    uint64_t *pd = (uint64_t*)dst;

    for (size_t i=0;i<nloop; i++) {
        pd[i] = 0;
    }
}

static bool T() {
    return true;
}

typedef uint64_t vec128i __attribute__((vector_size(16)));

static uint64_t gccvec128_load_test(void const  *src, size_t sz) {
    size_t nloop = sz/16;
    const vec128i *p = (const vec128i*)src;
    vec128i sum = {0,0};

    for (size_t i=0;i<nloop; i++) {
        sum += p[i];
    }
    return sum[0] + sum[1];
}
static void gccvec128_copy_test(void *dst, void const  *src, size_t sz) {
    size_t nloop = sz/16;
    const vec128i *ps = (const vec128i*)src;
    vec128i *pd = (vec128i*)dst;

    for (size_t i=0;i<nloop; i++) {
        pd[i] = ps[i];
    }
}
static void gccvec128_store_test(void *dst, size_t sz) {
    size_t nloop = sz/16;
    vec128i *pd = (vec128i*)dst;

    vec128i zero = {0,0};

    for (size_t i=0;i<nloop; i++) {
        pd[i] = zero;
    }
}


static const CopyTest copy_tests[] = {
    {"simple-long-copy", simple_long_copy_test, T},
    {"libc-memcpy", memcpy_test, T},
    {"gccvec128-copy", gccvec128_copy_test, T},

#ifdef X86
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
    {"avx256-store", avx256_store, have_avx},
    {"avx512-store", avx512_store, have_avx512f},

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

} // namespace

#if 0

#define compiler_mb() __asm__ __volatile__("" ::: "memory");
#define cpu_rmb() _mm_lfence()
#define cpu_wmb() _mm_sfence()

struct __attribute__((aligned(64))) thread_shared {
    pthread_t t;

    int ti;
    char *src;
    char *dst;
    size_t max_size;

    int start;
    int end;

    opfn_t op_fn;
    int num_iter;
    size_t copy_size;
};

static void run_test1(char *dstp, char *srcp, opfn_t opfn, int num_iter,
                      size_t copy_size) {
    for (int ii = 0; ii < num_iter; ii++) {
        opfn(dstp, srcp, copy_size);
    }
}

void *thread_fn(void *p) {
    struct thread_shared *ts = (struct thread_shared *)p;

    char *srcp = ts->src + ts->ti * ts->max_size;
    char *dstp = ts->dst + ts->ti * ts->max_size;

    while (1) {
        if (ts->start == 2) {
            break;
        }

        if (ts->start != 1) {
            compiler_mb();
            continue;
        }

        ts->start = 0;

        cpu_rmb();

        run_test1(dstp, srcp, ts->op_fn, ts->num_iter, ts->copy_size);

        cpu_wmb();

        ts->end = 1;
    }

    return NULL;
}

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

void *libc_memset(void *dst, const void *src, size_t sz) {
    memset(dst, 0, sz);
    return NULL;
}

void *amd_clzero(void *dst, const void *src, size_t sz) {
    size_t line_size = 64;
    size_t num_line = sz / line_size;

    unsigned char *d = (unsigned char *)dst;
    unsigned char *s = (unsigned char *)src;

    for (int i = 0; i < num_line; i++) {
        _mm_clzero(d);

        d += line_size;
    }

    return NULL;
}

void *sse_load(void *dst, const void *src, size_t sz) {
    size_t sz_sse = sz / 16;
    //__m128 *vdst = dst;
    const volatile __m128 *vsrc = src;

    for (size_t i = 0; i < sz_sse; i += 8) {
        vsrc[i + 0];
        vsrc[i + 1];
        vsrc[i + 2];
        vsrc[i + 3];

        vsrc[i + 4];
        vsrc[i + 5];
        vsrc[i + 6];
        vsrc[i + 7];
    }
    return NULL;
}

void *sse_store(void *dst, const void *src, size_t sz) {
    size_t sz_sse = sz / 16;
    __m128 *vdst = dst;
    // const volatile __m128 *vsrc = src;

    for (size_t i = 0; i < sz_sse; i += 8) {
        vdst[i + 0] = _mm_setzero_ps();
        vdst[i + 1] = _mm_setzero_ps();
        vdst[i + 2] = _mm_setzero_ps();
        vdst[i + 3] = _mm_setzero_ps();

        vdst[i + 4] = _mm_setzero_ps();
        vdst[i + 5] = _mm_setzero_ps();
        vdst[i + 6] = _mm_setzero_ps();
        vdst[i + 7] = _mm_setzero_ps();
    }
    return NULL;
}

void *avx_load(void *dst, const void *src, size_t sz) {
    size_t sz_sse = sz / 32;
    //__m128 *vdst = dst;
    const volatile __m256 *vsrc = src;

    for (size_t i = 0; i < sz_sse; i += 4) {
        vsrc[i + 0];
        vsrc[i + 1];
        vsrc[i + 2];
        vsrc[i + 3];
    }
    return NULL;
}

void *avx_store(void *dst, const void *src, size_t sz) {
    size_t sz_sse = sz / 32;
    __m256 *vdst = dst;
    // const volatile __m128 *vsrc = src;

    for (size_t i = 0; i < sz_sse; i += 4) {
        vdst[i + 0] = _mm256_setzero_ps();
        vdst[i + 1] = _mm256_setzero_ps();
        vdst[i + 2] = _mm256_setzero_ps();
        vdst[i + 3] = _mm256_setzero_ps();
    }
    return NULL;
}

void *sse_stream_store(void *dst, const void *src, size_t sz) {
    size_t sz_sse = sz / 16;
    __m128 *vdst = dst;
    // const volatile __m128 *vsrc = src;

    for (size_t i = 0; i < sz_sse; i += 4) {
        _mm_stream_ps((float *)&vdst[i + 0], _mm_setzero_ps());
        _mm_stream_ps((float *)&vdst[i + 1], _mm_setzero_ps());
        _mm_stream_ps((float *)&vdst[i + 2], _mm_setzero_ps());
        _mm_stream_ps((float *)&vdst[i + 3], _mm_setzero_ps());
    }
    return NULL;
}

void *rep_movsb(void *dst, const void *src, size_t sz) {
    asm volatile("rep movsb"
                 : "=D"(dst), "=S"(src), "=c"(sz)
                 : "0"(dst), "1"(src), "2"(sz)
                 : "memory");
    return NULL;
}

void *rep_stosb(void *dst, const void *src, size_t sz) {
    asm volatile("rep stosb"
                 : "=D"(dst), "=S"(src), "=c"(sz)
                 : "0"(dst), "1"(src), "2"(sz)
                 : "memory");
    return NULL;
}

int main(int argc, char **argv) {
    int nproc = sysconf(_SC_NPROCESSORS_ONLN);
    size_t max_size = 128 * 1024 * 1024;

    int opt;

    nproc -= 2;
    if (nproc <= 1) {
        nproc = 1;
    }

    while ((opt = getopt(argc, argv, "t:s:")) != -1) {
        switch (opt) {
        case 't':
            nproc = atoi(optarg);
            break;

        case 's':
            max_size = atoi(optarg) * 1024 * 1024;
            break;

        default:
            fprintf(stderr, "Usage: %s [-t num_thread] [-s size[MB]]\n",
                    argv[0]);
            return 1;
        }
    }

    char *src = mmap(0, max_size * nproc, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_POPULATE | MAP_ANONYMOUS, 0, 0);
    char *dst = mmap(0, max_size * nproc, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_POPULATE | MAP_ANONYMOUS, 0, 0);

    if (src == MAP_FAILED || dst == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    memset(src, 0, max_size * nproc);
    memset(dst, 0, max_size * nproc);

    struct thread_shared *ts = calloc(1, sizeof(*ts) * nproc - 1);

    for (int ti = 0; ti < nproc - 1; ti++) {

        ts[ti].ti = ti + 1;
        ts[ti].src = src;
        ts[ti].dst = dst;
        ts[ti].max_size = max_size;

        pthread_create(&ts[ti].t, NULL, thread_fn, &ts[ti]);
    }

    do_test(ts, dst, src, max_size, nproc, libc_memset, "libc-memset", 1);
    do_test(ts, dst, src, max_size, nproc, memcpy, "libc-memcpy", 2);
    do_test(ts, dst, src, max_size, nproc, sse_load, "sse-load", 1);
    do_test(ts, dst, src, max_size, nproc, sse_store, "sse-store", 1);
    do_test(ts, dst, src, max_size, nproc, sse_store, "sse-stream-store", 1);
    do_test(ts, dst, src, max_size, nproc, avx_load, "avx-load", 1);
    do_test(ts, dst, src, max_size, nproc, avx_store, "avx-store", 1);
    do_test(ts, dst, src, max_size, nproc, rep_movsb, "rep-movsb", 2);
    do_test(ts, dst, src, max_size, nproc, rep_stosb, "rep-stosb", 1);
    do_test(ts, dst, src, max_size, nproc, amd_clzero, "amd_clzero", 1);

    for (int ti = 0; ti < nproc - 1; ti++) {
        ts[ti].start = 2;

        pthread_join(ts[ti].t, NULL);
    }
}

#endif

struct MemoryBandwidth : public BenchDesc {
    typedef Table1D<double, std::string> table_t;

    MemoryBandwidth(bool full_thread)
        : BenchDesc(full_thread ? "memory-bandwidth full thread (16Mib/thread)"
                                : "memory-bandwidth 1thread (16Mib)") {}

    virtual result_t run(GlobalState *g) override {
        int ncopy_test = sizeof(copy_tests) / sizeof(copy_tests[0]);
        int nload_test = sizeof(load_tests) / sizeof(load_tests[0]);
        int nstore_test = sizeof(store_tests) / sizeof(store_tests[0]);

        int ntest = ncopy_test + nload_test + nstore_test;

        std::vector<std::string> test_name_list;

        for (int i=0; i<ncopy_test; i++) {
            test_name_list.push_back(copy_tests[i].name);
        }
        for (int i=0; i<nload_test; i++) {
            test_name_list.push_back(load_tests[i].name);
        }
        for (int i=0; i<nstore_test; i++) {
            test_name_list.push_back(store_tests[i].name);
        }

        table_t *result = new table_t("test_name", ntest);
        result->column_label = "MiB/sec";
        // result->row_label = cores;

        return result_t(result);
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

} // namespace smbm