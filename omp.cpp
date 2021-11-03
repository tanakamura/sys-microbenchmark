
#include "cpuset.h"
#include "oneshot_timer.h"
#include "simple-run.h"
#include "sys-microbenchmark.h"
#include "table.h"

#ifdef _OPENMP
#include <omp.h>
#include <unistd.h>
#endif

namespace smbm {

namespace {

#ifdef _OPENMP
struct no_arg {
    void *alloc_arg() { return nullptr; };
    void free_arg(void *p) {}
};

struct omp_get_thread_num1 : public no_arg {
    void run(void *arg) { omp_get_thread_num(); }
};
struct omp_get_num_threads1 : public no_arg {
    void run(void *arg) { omp_get_num_threads(); }
};

struct parallel : public no_arg {
    void run(void *arg) {

#pragma omp parallel
        { asm volatile(" " ::: "memory"); }
    }
};

struct parallel_for_nthread : public no_arg {
    void run(void *arg) {
        int nthread = omp_get_num_threads();

#pragma omp parallel for
        for (int i = 0; i < nthread; i++) {
            asm volatile(" " ::: "memory");
        }
    }
};

struct parallel_for_64K : public no_arg {
    void run(void *arg) {

#pragma omp parallel for
        for (int i = 0; i < 65536; i++) {
            asm volatile(" " ::: "memory");
        }
    }
};

struct parallel_for_1M : public no_arg {
    void run(void *arg) {

#pragma omp parallel for
        for (int i = 0; i < 1048576; i++) {
            asm volatile(" " ::: "memory");
        }
    }
};

struct parallel_task_spawn : public no_arg {
    void run(void *arg) {

#pragma omp parallel
        {
            if (omp_get_thread_num() == 0) {
#pragma omp task
                asm volatile(" " ::: "memory");
            }
        }

        return;
    }
};

struct parallel_task_spawn_1K : public no_arg {
    void run(void *arg) {
#pragma omp parallel
        {
            if (omp_get_thread_num() == 0) {
                for (int i = 0; i < 1024; i++) {
#pragma omp task
                    asm volatile(" " ::: "memory");
                }
            }
        }

        return;
    }
};

struct parallel_barrier : public no_arg {
    void run(void *arg) {
#pragma omp parallel
        {
            asm volatile(" " ::: "memory");

#pragma omp barrier
            asm volatile(" " ::: "memory");

            asm volatile(" " ::: "memory");
        }
    }
};

struct parallel_barrier_1K : public no_arg {
    void run(void *arg) {
#pragma omp parallel
        {
            asm volatile(" " ::: "memory");

            for (int i = 0; i < 1024; i++) {
#pragma omp barrier
                asm volatile(" " ::: "memory");
            }

            asm volatile(" " ::: "memory");
        }
    }
};


#define FOR_EACH_TEST(F)                                                       \
    /*F(nop)*/                                                                 \
    F(omp_get_thread_num1)                                                     \
    F(omp_get_num_threads1)                                                    \
    F(parallel)                                                                \
    F(parallel_for_nthread)                                                    \
    F(parallel_for_64K)                                                        \
    F(parallel_for_1M)                                                         \
    F(parallel_task_spawn)                                                     \
    F(parallel_task_spawn_1K)                                                  \
    F(parallel_barrier)                                                        \
    F(parallel_barrier_1K)


#endif

struct OpenMP : public BenchDesc {
    OpenMP() : BenchDesc("OpenMP") {}

    virtual result_t run(GlobalState const *g) override {

#ifdef _OPENMP
        bind_self_to_all(g->proc_table);

        int count = 0;
#define INC_COUNT(F) count++;
        FOR_EACH_TEST(INC_COUNT);

        typedef Table1D<double, std::string> result_t;
        result_t *result = new result_t("test_name", count);

#define NAME(F) #F,

        result->column_label = " nsec/iter";
        result->row_label = std::vector<std::string>{FOR_EACH_TEST(NAME)};

        count = 0;

#define RUN(F)                                                                 \
    {                                                                          \
        F t;                                                                   \
        (*result)[count++] = run_test(g, &t);                                  \
    }
        FOR_EACH_TEST(RUN)

        bind_self_to_first(g->proc_table, true);

        return std::unique_ptr<BenchResult>(result);

#else
        return result_t();
#endif
    }
    virtual result_t parse_json_result(picojson::value const &v) override {
        return result_t(Table1D<double, std::string>::parse_json_result(v));
    }

    bool available(const GlobalState *g) override {
#ifdef _OPENMP
        return true;
#else
        return false;
#endif
    }

};

} // namespace

std::unique_ptr<BenchDesc> get_openmp_desc() {
    return std::unique_ptr<BenchDesc>(new OpenMP());
}

} // namespace smbm
