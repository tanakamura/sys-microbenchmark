#include "simple-run.h"
#include "sys-microbenchmark.h"
#include "table.h"

namespace smbm {

namespace {

struct STATE {
    double x;
    double y;
    double z;
};

static double
l2d(uint64_t v)
{
    union {
        double d;
        uint64_t l;
    }u;
    u.l = v;
    return u.d;
}

struct base {
    double x, y, z;
    base(double x, double y, double z)
        :x(x), y(y), z(z)
    {
    }
    STATE *alloc_arg() {
        STATE *p = new STATE;
        p->x = x;
        p->y = y;
        p->z = z;
        return p;
    }

    void free_arg(STATE *s) {
        delete s;
    }

};

struct normal_add
    :public base
{
    normal_add()
        :base(1.0, 2.0, 3.0) 
    {}

    void run(const GlobalState *g, STATE *s) {
        g->dummy_write(0, s->x + s->y);
        compiler_mb();
    }
};
struct denormal_add
    :public base
{
    denormal_add()
        :base(l2d(1), l2d(2), l2d(3))
    {}

    void run(const GlobalState *g, STATE *s) {
        g->dummy_write(0, s->x + s->y);
        compiler_mb();
    }
};

struct normal_mul
    :public base
{
    normal_mul()
        :base(1.0, 2.0, 3.0) 
    {}

    void run(const GlobalState *g, STATE *s) {
        g->dummy_write(0, s->x * s->y);
        compiler_mb();
    }
};
struct denormal_mul
    :public base
{
    denormal_mul()
        :base(l2d(1), l2d(2), l2d(3))
    {}

    void run(const GlobalState *g, STATE *s) {
        g->dummy_write(0, s->x * s->y);
        compiler_mb();
    }
};

struct normal_div
    :public base
{
    normal_div()
        :base(1.0, 2.0, 3.0) 
    {}

    void run(const GlobalState *g, STATE *s) {
        g->dummy_write(0, s->x / s->y);
        compiler_mb();
    }
};
struct denormal_div
    :public base
{
    denormal_div()
        :base(l2d(1), l2d(2), l2d(3))
    {}

    void run(const GlobalState *g, STATE *s) {
        g->dummy_write(0, s->x / s->y);
        compiler_mb();
    }
};



#define FOR_EACH_TEST(F)                        \
    F(denormal_add)                                 \
    F(normal_add)                                 \
    F(denormal_mul)                                 \
    F(normal_mul)                                   \
    F(denormal_div)                                 \
    F(normal_div)                                   \

typedef Table1DBenchDesc<double,std::string> parent_t;
template <bool static_available>
struct FPU : public parent_t{
    bool cycle;
    FPU(bool cycle) : parent_t(cycle?"fpu-cycle":"fpu-realtime"), cycle(cycle) {}

    virtual result_t run(GlobalState const *g) override {
        int count = 0;
#define INC_COUNT(F) count++;
        FOR_EACH_TEST(INC_COUNT);

        table_t *result = new table_t("test_name", count);

#define NAME(F) #F,

        if (cycle) {
            result->column_label = " cycle/instruction";
        } else {
            result->column_label = " nsec/instruction";
        }
        result->row_label = std::vector<std::string>{FOR_EACH_TEST(NAME)};

        count = 0;

#ifdef HAVE_HW_PERF_COUNTER
#define RUN(F)                                                                 \
    {                                                                          \
        F t;                                                                   \
        if (cycle)                                                      \
            (*result)[count++] = run_test_g_cycle(g, &t);               \
        else                                                            \
            (*result)[count++] = run_test_g(g, &t);                     \
    }
#else
#define RUN(F)                                                                 \
    {                                                                          \
        F t;                                                            \
        (*result)[count++] = run_test_g(g, &t);                         \
    }
#endif

        FOR_EACH_TEST(RUN)

        return std::unique_ptr<BenchResult>(result);
    }

    bool available(const GlobalState *g) override {
        if (!static_available) {
            return false;
        }
        if (cycle) {
            if (! g->is_hw_perf_counter_available()) {
                return false;
            }
        }

        long v1 = 1;
        long v2 = 2;
        asm volatile (" " :"+r"(v1));
        asm volatile (" " :"+r"(v2));

        double x = l2d(v1);
        double y = l2d(v2);

        if ((x + y) == 0) {
            /* lacks denormal support */
            return false;
        }

        return true;
    }
};

} // namespace

std::unique_ptr<BenchDesc> get_fpu_realtime_desc() {
    return std::unique_ptr<BenchDesc>(new FPU<true>(false));
}
#ifdef HAVE_HW_PERF_COUNTER
std::unique_ptr<BenchDesc> get_fpu_cycle_desc() {
    return std::unique_ptr<BenchDesc>(new FPU<true>(true));
}
#else
std::unique_ptr<BenchDesc> get_fpu_cycle_desc() {
    return std::unique_ptr<BenchDesc>(new FPU<false>(true));
}
#endif

} // namespace smbm
