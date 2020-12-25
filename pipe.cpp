#include "sys-microbenchmark.h"
#include "table.h"
#include "memalloc.h"
#include "cpu-feature.h"
#include <algorithm>
#include <random>

namespace smbm {
namespace {

} // namespace
} // namespace smbm

#ifdef X86
#include "pipe-x86.h"
#else

#endif

namespace smbm {

namespace {

enum class PIPE {
    INT_ADD,
    FP_ADD,
    LOAD,

    NUM_PIPE
};

struct ResultValue {
    double nsec;
    double cycle;
};

struct Loop {
    ExecutableMemory m;
    virtual void body(char *&p) = 0;
    virtual void setup(char *&p) {}
    Loop()
    {
        m = alloc_exeutable(1024*1024);
    }
    ~Loop() {
        free_executable(&m);
    }

    virtual int loop_count() {
        return 1024;
    }
    void gen() {
        char *p = (char*)m.p;
        enter_frame(p, callee_saved_int_regs, callee_saved_fp_regs);

        setup(p);

        int lc = loop_count();

        if (lc > 1) {
            loop_start(p, loop_count());
        }
        char *loop_start = p;

        body(p);

        if (lc > 1) {
            loop_end(p, loop_start);
        }
        exit_frame(p);
    }

    void run_func() {
        typedef void (*fp_t)(void);
        fp_t func = (fp_t)m.p;
        func();
    }
};

struct IssueRate {
    double dual_issue_latency[(int)PIPE::NUM_PIPE];
};

struct measure_latency
    :public Loop
{
    PIPE pipe;
    int depth;
    static constexpr int inst_per_loop = 1024;
    int unroll;
    void **p;

    int loop_count() {
        return 128;
    }

    measure_latency(PIPE p, int depth)
        :pipe(p), depth(depth)
    {
        this->p = (void**)malloc(sizeof(void*));
        *this->p = (void*)this->p;
        this->unroll = 1;
    }
    ~measure_latency() {
        free(p);
    }

    void setup(char *&p) override {
        if (pipe == PIPE::LOAD) {
            gen_setimm64(p, 1, (uintptr_t)this->p);
            gen_setimm64(p, 2, (uintptr_t)this->p);
        }
    }

    void body(char *&p) {
        gen_iclear(p, 1);
        gen_iclear(p, 2);

        for (int i=0; i<depth; i++) {
            gen_iadd(p, 1, 1);
        }
        for (int i=0; i<depth; i++) {
            gen_iadd(p, 2, 2);            
        }

        for (int i=depth*3; i<inst_per_loop; i+=3) {
            gen_iadd(p, 1, 1);
            gen_iadd(p, 2, 2);
        }

        gen_iadd(p, 1, 2);
    }

    double run(GlobalState const *g) {
        /* warm up */
        run_func();

        uint64_t delta_min = 999999;
        if (g->is_hw_perf_counter_available()) {
            for (int i=0; i<16; i++) {
                auto c0 = g->get_hw_cpucycle();
                run_func();
                auto c1 = g->get_hw_cpucycle();

                delta_min = std::min(c1-c0, delta_min);
            }
            return delta_min / (double)(loop_count() * inst_per_loop);
        } else {
            for (int i=0; i<16; i++) {
                auto c0 = userland_timer_value::get();
                run_func();
                auto c1 = userland_timer_value::get();

                delta_min = std::min(c1-c0, delta_min);
            }
            return (1e9*g->userland_timer_delta_to_sec(delta_min)) / (double)(loop_count() * inst_per_loop);
        }
    }
};

struct __attribute__((aligned(CACHELINE_SIZE))) CacheLine {
    void *ptr;
    uint64_t idx;
};

struct Pipe : public BenchDesc {
    Pipe() : BenchDesc("pipe") {}

    typedef Table1D<double, std::string> table_t;
 
    virtual result_t run(GlobalState const *g) override {
        std::vector<std::string> labels;
        std::vector<double> results;
        IssueRate ir;
        const char *name_table[] = {"int add", "fp add", "load"};

        int nline = 4*1024*1024;
        std::vector<CacheLine> random_ptr(nline);

        for (int i=0; i<nline-1; i++) {
            random_ptr[i].idx = i;
        }
        random_ptr[nline-1].idx = 0;

        {
            std::mt19937 mt;
            mt.seed(0);
            std::shuffle(random_ptr.begin(), random_ptr.end(), mt);
        }

        for (int i=0; i<nline; i++) {
            int idx = random_ptr[i].idx;
            random_ptr[i].ptr = &random_ptr[idx];
        }

        for (int i=0; i<(int)PIPE::NUM_PIPE; i++) {
            measure_latency ml((PIPE)i, 1);
            ml.gen();
            double v = ml.run(g);
            labels.push_back(std::string("dual ") + name_table[i] + " latency [nsec]");
            results.push_back(v);
            ir.dual_issue_latency[i] = v;
        }

        for (int pi=0; pi<(int)1; pi++) {
            for (int di=1; di<=256; di++) {
                measure_latency ml((PIPE)pi, di);
                ml.gen();
                double v = ml.run(g);
                double base = ir.dual_issue_latency[pi];

                double rate = v / base;

                labels.push_back(std::string("dual ") + name_table[pi] + " latency (depth=" + std::to_string(di) + ") [current/base]");
                results.push_back(rate);

                if (rate > 1.10) {
                    //break;
                }
            }
        }

        table_t *table = new table_t("test_name", results.size());
        table->row_label = labels;
        table->column_label = "result";

        for (size_t i=0; i<results.size(); i++) {
            (*table)[i] = results[i];
        }

        return result_t(table);
    }

    bool available(const GlobalState *g) override {
#ifdef X86
        return g->is_hw_perf_counter_available();
#else
        return false;
#endif
    }

    int double_precision() {
        return 2;
    }

    virtual result_t parse_json_result(picojson::value const &v) {
        return result_t(table_t::parse_json_result(v));
    }

};

} // namespace

std::unique_ptr<BenchDesc> get_pipe_desc() {
    return std::unique_ptr<BenchDesc>(new Pipe());
}

} // namespace smbm
