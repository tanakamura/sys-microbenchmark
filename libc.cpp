#ifndef NO_SETJMP
#include <setjmp.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "oneshot_timer.h"
#include "simple-run.h"
#include "sys-microbenchmark.h"
#include "table.h"

namespace smbm {

namespace {

struct no_arg {
    void *alloc_arg() { return nullptr; };
    void free_arg(void *p) {}
};

// struct nop : public no_arg {
//    void run(void *arg) {  }
//};

struct atoi_99999 : public no_arg {
    void run(GlobalState const *g, void *arg) {
        g->dummy_write(0, atoi("99999"));
    }
};
struct fflush_stdout : public no_arg {
    void run(GlobalState const *g, void *arg) { fflush(stdout); }
};
struct sscanf_double_99999 : public no_arg {
    void run(GlobalState const *g, void *arg) {
        char buffer[] = "99999.0";
        double v = 0;

        sscanf(buffer, "%lf", &v);
        if (v != 99999.0) {
            abort();
        }

        g->dummy_write(0, v);
    }
};
struct sscanf_int_99999 : public no_arg {
    void run(GlobalState const *g, void *arg) {
        char buffer[] = "99999";
        int v = 0;

        sscanf(buffer, "%d", &v);
        if (v != 99999) {
            abort();
        }

        g->dummy_write(0, v);
    }
};
struct sprintf_double_99999 : public no_arg {
    void run(GlobalState const *g, void *arg) {
        char buffer[1024];
        double v = g->getzero();

        int r = sprintf(buffer, "%f", v + 99999.0);
        if (r < 0) {
            abort();
        }

        g->dummy_write(0, buffer[0]);
    }
};
struct sprintf_double_033333 : public no_arg {
    void run(GlobalState const *g, void *arg) {
        char buffer[1024];
        double v = g->getzero();

        int r = sprintf(buffer, "%f", v + (1.0 / 3.0));
        if (r < 0) {
            abort();
        }

        g->dummy_write(0, buffer[0]);
    }
};
struct sprintf_int_99999 : public no_arg {
    void run(GlobalState const *g, void *arg) {
        char buffer[1024];
        int v = g->getzero();

        int r = sprintf(buffer, "%d", v + 99999);
        if (r < 0) {
            abort();
        }

        g->dummy_write(0, buffer[0]);
    }
};
struct malloc_free_1byte : public no_arg {
    void run(GlobalState const *g, void *arg) {
        void *p = malloc(1);
        g->dummy_write(0, (uintptr_t)p);
        free(p);
    }
};
struct malloc_free_1MiB : public no_arg {
    void run(GlobalState const *g, void *arg) {
        void *p = malloc(1);
        g->dummy_write(0, (uintptr_t)p);
        free(p);
    }
};
struct cos1 : public no_arg {
    void run(GlobalState const *g, void *arg) {
        double z = g->getzero();
        double v = cos(z + 1);
        g->dummy_write(0, v);
    }
};
struct cos0 : public no_arg {
    void run(GlobalState const *g, void *arg) {
        double z = g->getzero();
        double v = cos(z);
        g->dummy_write(0, v);
    }
};
struct cosPI : public no_arg {
    void run(GlobalState const *g, void *arg) {
        double z = g->getzero();
        double v = cos(z + M_PI);
        g->dummy_write(0, v);
    }
};
struct exp : public no_arg {
    void run(GlobalState const *g, void *arg) {
        double z = g->getzero();
        double v = ::exp(z + 352423424980.43242);
        g->dummy_write(0, v);
    }
};
struct log10 : public no_arg {
    void run(GlobalState const *g, void *arg) {
        double z = g->getzero();
        double v = ::log10(z + 352423424980.43242);
        g->dummy_write(0, v);
    }
};
struct log : public no_arg {
    void run(GlobalState const *g, void *arg) {
        double z = g->getzero();
        double v = ::log(z + 352423424980.43242);
        g->dummy_write(0, v);
    }
};
struct log2 : public no_arg {
    void run(GlobalState const *g, void *arg) {
        double z = g->getzero();
        double v = ::log2(z + 352423424980.43242);
        g->dummy_write(0, v);
    }
};
struct rand : public no_arg {
    void run(GlobalState const *g, void *arg) { g->dummy_write(0, ::rand()); }
};

#ifndef NO_SETJMP
struct setjmp1 : public no_arg {
    void run(GlobalState const *g, void *arg) {
        jmp_buf jb;
        g->dummy_write(0, ::setjmp(jb));
    }
};
#endif

static void call_va_start_end(GlobalState const *g, int x, ...) {
    va_list va;
    va_start(va, x);
    g->dummy_write(0, va_arg(va, int));
    va_end(va);
}

struct va_start_end : public no_arg {
    void run(GlobalState const *g, void *arg) {
        call_va_start_end(g, 1, 2, 3, 4);
    }
};

struct clock : public no_arg {
    void run(GlobalState const *g, void *arg) { g->dummy_write(0, ::clock()); }
};
struct time : public no_arg {
    void run(GlobalState const *g, void *arg) {
        g->dummy_write(0, ::time(NULL));
    }
};
struct gmtime {
    void *alloc_arg() {
        time_t *v = (time_t *)malloc(sizeof(time_t));
        ::time(v);
        return v;
    };
    void free_arg(void *p) { free(p); }

    void run(GlobalState const *g, void *arg) {
        time_t *v = (time_t *)arg;
        struct tm *t = ::gmtime(v);
        g->dummy_write(0, t->tm_year);
    }
};
struct localtime {
    void *alloc_arg() {
        time_t *v = (time_t *)malloc(sizeof(time_t));
        ::time(v);
        return v;
    };
    void free_arg(void *p) { free(p); }

    void run(GlobalState const *g, void *arg) {
        time_t *v = (time_t *)arg;
        struct tm *t = ::localtime(v);
        g->dummy_write(0, t->tm_year);
    }
};
struct mktime {
    void *alloc_arg() {
        struct tm *v = (struct tm *)malloc(sizeof(struct tm));
        time_t now = ::time(NULL);
        *v = *::gmtime(&now);
        return v;
    };
    void free_arg(void *p) { free(p); }

    void run(GlobalState const *g, void *arg) {
        struct tm *t = (struct tm *)arg;
        time_t v = ::mktime(t);
        g->dummy_write(0, v);
    }
};
struct asctime {
    void *alloc_arg() {
        struct tm *v = (struct tm *)malloc(sizeof(struct tm));
        time_t now = ::time(NULL);
        *v = *::gmtime(&now);
        return v;
    };
    void free_arg(void *p) { free(p); }

    void run(GlobalState const *g, void *arg) {
        struct tm *t = (struct tm *)arg;
        char *v = ::asctime(t);
        g->dummy_write(0, v[0]);
    }
};

struct SortData {
    int *orig;
    int *cur;
};

static int int_compare(const void *l, const void *r) {
    int lv = *(int *)l;
    int rv = *(int *)r;

    return lv < rv;
}

struct sort {
    int size;

    sort(int size) : size(size) {}

    void *alloc_arg() {
        int *p = new int[size];
        for (int i = 0; i < size; i++) {
            p[i] = drand48() * 1024 * 1024 * 16;
        }

        struct SortData *ret = new SortData;
        ret->orig = p;
        ret->cur = new int[size];

        return ret;
    }

    void free_arg(void *p) {
        struct SortData *d = (struct SortData *)p;

        delete[] d->orig;
        delete[] d->cur;

        delete d;
    }

    void run(GlobalState const *g, void *arg) {
        struct SortData *d = (struct SortData *)arg;

        memcpy(d->cur, d->orig, sizeof(int) * size);

        qsort(d->cur, size, sizeof(int), int_compare);
    }
};

struct sort1K : public sort {
    sort1K() : sort(1024) {}
};
struct sort4 : public sort {
    sort4() : sort(4) {}
};

#define FOR_EACH_TEST_LIBC_BASE(F0,F1)                                       \
    F0(atoi_99999)                                                              \
    F0(fflush_stdout)                                                           \
    F0(sscanf_double_99999)                                                     \
    F0(sscanf_int_99999)                                                        \
    F0(sprintf_double_99999)                                                    \
    F0(sprintf_double_033333)                                                   \
    F0(sprintf_int_99999)                                                       \
    F0(malloc_free_1byte)                                                       \
    F0(malloc_free_1MiB)                                                        \
    F0(sort1K)                                                                  \
    F0(sort4)                                                                   \
    F0(cos1)                                                                    \
    F0(cos0)                                                                    \
    F0(cosPI)                                                                   \
    F0(exp)                                                                     \
    F0(log)                                                                     \
    F0(log10)                                                                   \
    F0(log2)                                                                    \
    F0(rand)                                                                    \
    F0(va_start_end)                                                            \
    F0(clock)                                                                   \
    F0(time)                                                                    \
    F0(gmtime)                                                                  \
    F0(localtime)                                                               \
    F0(mktime)                                                                  \
    F1(asctime)


#define IGNORE(X)

#ifdef WASI
#define FOR_EACH_TEST_LIBC(F) FOR_EACH_TEST_LIBC_BASE(F,IGNORE)
#else
#define FOR_EACH_TEST_LIBC(F) FOR_EACH_TEST_LIBC_BASE(F,F)
#endif

#define FOR_EACH_TEST_SETJMP(F) F(setjmp1)

#ifdef NO_SETJMP
#define FOR_EACH_TEST(F)                        \
    FOR_EACH_TEST_LIBC(F)                       \

#else
#define FOR_EACH_TEST(F)                        \
    FOR_EACH_TEST_LIBC(F)                       \
    FOR_EACH_TEST_SETJMP(F)

#endif

typedef Table1DBenchDesc<double, std::string> parent_t;

struct LIBC : public parent_t {
    LIBC() : parent_t("libc") {}

    virtual result_t run(GlobalState const *g) override {
        int count = 0;
#define INC_COUNT(F) count++;
        FOR_EACH_TEST(INC_COUNT);

        table_t *result = new table_t("test_name", count);

#define NAME(F) #F,

        result->column_label = " nsec/call";
        result->row_label = std::vector<std::string>{FOR_EACH_TEST(NAME)};

        count = 0;

#define RUN(F)                                                                 \
    {                                                                          \
        F t;                                                                   \
        (*result)[count++] = run_test_g(g, &t);                                \
    }
        FOR_EACH_TEST(RUN)

        return std::unique_ptr<BenchResult>(result);
    }
};

} // namespace

std::unique_ptr<BenchDesc> get_libc_desc() {
    return std::unique_ptr<BenchDesc>(new LIBC());
}

} // namespace smbm
