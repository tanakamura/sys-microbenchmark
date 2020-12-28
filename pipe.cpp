/* basic idea from https://blog.stuffedcow.net/2013/05/measuring-rob-capacity/
 */

#include "cpu-feature.h"
#include "memalloc.h"
#include "sys-microbenchmark.h"
#include "table.h"
#include <algorithm>
#include <limits.h>
#include <random>

#include "barrier.h"

#ifdef X86
#include "pipe-x86.h"

#elif defined AARCH64
#include "pipe-aarch64.h"

#else

#endif

namespace smbm {

#if (defined X86) || (defined AARCH64)

namespace {

enum class Buffer {
    ROB,
    INT_PRF,
    FP_PRF,
    // INT_WINDOW,
    // FP_WINDOW,

    NUM_BUFFER
};

enum class SchedPipe {
    INT_ADD,
    INT_MUL,
    FP,
    //    LD,
    NUM_PIPE,
};

const char *name_table[] = {"ROB", "INT PRF",
                            "FP PRF"}; //, "INT_WINDOW", "FP_WINDOW"};
const char *pipe_name_table[] = {"INT_ADD", "INT_MUL", "FP", "LD"};

struct Loop {
    ExecutableMemory m;
    virtual void body(char *&p) = 0;
    virtual void setup(char *&p) {}
    Loop() { m = alloc_exeutable(1024 * 1024 * 16); }
    ~Loop() { free_executable(&m); }

    virtual int loop_count() { return 1024; }
    void gen() {
        char *p = (char *)m.p;
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
        exit_frame(p, callee_saved_int_regs, callee_saved_fp_regs);

        isync(m.p, p);
    }

    void run_func() {
        typedef void (*fp_t)(void);
        fp_t func = (fp_t)m.p;
        func();
    }
};

struct Chain {
    uint64_t v;
};

struct BufferEstimator : public Loop {
    Buffer pipe;
    int depth;
    static constexpr int window_per_loop = 16;
    void **p;
    Chain *random_data0, *random_data1;
    static constexpr int load_chain_num = 4;

    int loop_counter_reg, p0_reg, p1_reg, p2_reg;

    int loop_count() { return 1024; }

    BufferEstimator(Buffer p, int depth, Chain *l, Chain *l2)
        : pipe(p), depth(depth), random_data0(l), random_data1(l2) {
        this->p = (void **)malloc(sizeof(void *));
        *this->p = (void *)this->p;

        loop_counter_reg = simple_regs[0];
        p0_reg = simple_regs[1];
        p1_reg = simple_regs[2];
        p2_reg = simple_regs[3];
    }
    ~BufferEstimator() { free(p); }

    void setup(char *&p) override {
        gen_setimm64(p, p0_reg, (uintptr_t)this->random_data0);
        gen_setimm64(p, p1_reg, (uintptr_t)this->random_data1);
        gen_setimm64(p, p2_reg, (uintptr_t)this->p);
    }

    void gen_target(char *&p, int cur_ld) {
        for (int i = 0; i < depth; i++) {
            int nreg = simple_regs.size() - 4;
            int reg = simple_regs[(i % nreg) + 4];

            switch (pipe) {
            case Buffer::ROB:
                gen_nop(p);
                break;
            case Buffer::INT_PRF:
                gen_iadd(p, reg, reg);
                break;
            case Buffer::FP_PRF:
                gen_fadd(p, reg, reg);
                break;
                //            case Buffer::INT_WINDOW:
                //                if (i == 0) {
                //                    gen_iadd(p, simple_regs[4], cur_ld);
                //                } else {
                //                    gen_iadd(p, simple_regs[4],
                //                    simple_regs[4]);
                //                }
                //                break;
                //            case Buffer::FP_WINDOW:
                //                if (i == 0) {
                //                    gen_int_to_fp(p, simple_regs[4], cur_ld);
                //                } else {
                //                    gen_fadd(p, simple_regs[4],
                //                    simple_regs[4]);
                //                }
                //                break;

            default:
                break;
            }
        }

        for (int i = 0; i < depth; i++) {
            switch (pipe) {
                //            case Buffer::INT_WINDOW:
                //                if (i == 0) {
                //                    gen_iadd(p, simple_regs[5], cur_ld);
                //                } else {
                //                    gen_iadd(p, simple_regs[5],
                //                    simple_regs[5]);
                //                }
                //                break;
                //
                //            case Buffer::FP_WINDOW:
                //                if (i == 0) {
                //                    gen_int_to_fp(p, simple_regs[5], cur_ld);
                //                } else {
                //                    gen_fadd(p, simple_regs[5],
                //                    simple_regs[5]);
                //                }
                //                break;
            default:
                break;
            }
        }
    }

    void body(char *&p) {
        for (int i = 0; i < window_per_loop; i++) {
            for (int i=0; i<load_chain_num; i++) {
                gen_load64(p, p0_reg, p0_reg);
            }
            gen_target(p, p0_reg);
            for (int i=0; i<load_chain_num; i++) {
                gen_load64(p, p1_reg, p1_reg);
            }
            gen_target(p, p1_reg);
        }
    }

    void clear_chain_cache() {
        size_t cache_size = 64 * 1024 * 1024;
        char *ptr = (char *)malloc(cache_size);
        memset(ptr, 0, cache_size);
        free(ptr);
    }

    double run(GlobalState const *g) {
        /* warm up */
        run_func();

        int ntry = 4;
        uint64_t min = ULONG_LONG_MAX;

        if (g->is_hw_perf_counter_available()) {
            for (int i = 0; i < ntry; i++) {
                clear_chain_cache();

                auto c0 = g->get_hw_cpucycle();
                run_func();
                auto c1 = g->get_hw_cpucycle();

                auto d = c1-c0;
                min = std::min(min,d);
            }

            return min / (double)(loop_count() * window_per_loop);
        } else {
            for (int i = 0; i < ntry; i++) {
                clear_chain_cache();

                auto c0 = userland_timer_value::get();
                run_func();
                auto c1 = userland_timer_value::get();

                auto d = c1-c0;

                min = std::min(min,d);
            }

            return (1e9 * g->userland_timer_delta_to_sec(min)) /
                   (double)(loop_count() * window_per_loop);
        }
    }
};

struct SchedEstimator : public Loop {
    static constexpr int dependency_length = 512*3;
    SchedPipe pipe;
    int overlap;
    bool multi_chain;

    SchedEstimator(SchedPipe p, int overlap, bool multi_chain)
        : pipe(p), overlap(overlap), multi_chain(multi_chain) {}

    void gen_target(char *&p, int operand0, int operand1) {
        switch (pipe) {
        case SchedPipe::INT_ADD:
            gen_iadd(p, operand0, operand1);
            break;
        case SchedPipe::INT_MUL:
            gen_imul(p, operand0, operand1);
            break;

        case SchedPipe::FP:
            gen_fadd(p, operand0, operand1);
            break;

        default:
            break;
        }
    }

    int loop_count() { return 4096; }

    void body(char *&p) {
        int overlap2 = overlap * 2;
        if (!multi_chain) {
            overlap2++;
        }
        int chain_half = dependency_length / 2;
        int overlap_half = overlap2 / 2;

        int reg0 = simple_regs[0 + 4];
        int reg1 = simple_regs[1 + 4];

        int head_length = chain_half - overlap_half;
        int tail_length = dependency_length - (head_length + overlap2);

        for (int i = 0; i < head_length; i++) {
            gen_target(p, reg0, reg0);
        }

        for (int i = 0; i < overlap; i++) {
            if (i == 0 && !multi_chain) {
                gen_target(p, reg1, reg0);
            } else {
                gen_target(p, reg1, reg1);
            }
        }

        for (int i = 0; i < overlap; i++) {
            gen_target(p, reg0, reg0);
        }

        if (!multi_chain) {
            gen_target(p, reg0, reg1);
        }

        for (int i = 0; i < tail_length; i++) {
            gen_target(p, reg0, reg0);
        }
    }

    double run(GlobalState const *g) {
        /* warm up */
        run_func();

        int ntry = 4;
        uint64_t min = -1ULL;

        if (g->is_hw_perf_counter_available()) {
            for (int i = 0; i < ntry; i++) {
                auto c0 = g->get_hw_cpucycle();
                run_func();
                auto c1 = g->get_hw_cpucycle();

                auto d = c1-c0;
                min = std::min(d,min);
            }

            return min / (double)loop_count();
        } else {
            for (int i = 0; i < ntry; i++) {
                auto c0 = userland_timer_value::get();
                run_func();
                auto c1 = userland_timer_value::get();

                auto d = c1-c0;
                min = std::min(d,min);
            }

            return (1e9 * g->userland_timer_delta_to_sec(min)) /
                   (double)loop_count();
        }
    }
};

static inline int
reverse_bit(int bits, int nbits)
{
    int ret = 0;
    for (int i=0; i<nbits; i++) {
        if (bits & (1<<i)) {
            ret |= 1<<(nbits-i-1);
        }
    }

    return ret;
}

struct Pipe : public BenchDesc {
    Pipe() : BenchDesc("pipe") {}

    typedef Table1D<int, std::string> table_t;

    virtual result_t run(GlobalState const *g) override {
        std::vector<std::string> labels;
        std::vector<int> results;

        int max_depth = 400;

        if (1) {
            int nchain = 1024*1024*16;
            std::vector<Chain> random_ptr(nchain);

            int pagesize = 4096;
            int npage = (nchain*sizeof(Chain)) / pagesize;

            int chain_per_line = CACHELINE_SIZE / sizeof(Chain);
            int line_per_page = pagesize / CACHELINE_SIZE;

            int npage_bit = __builtin_ctz(npage);
            int lpp_bit = __builtin_ctz(line_per_page);
            int cpl_bit = __builtin_ctz(chain_per_line);

            auto shuf_bit = [npage_bit,lpp_bit,cpl_bit](int bits) {
#if 0
                int ci = bits & ((1<<cpl_bit)-1);

                bits >>= cpl_bit;
                int li = bits & ((1<<lpp_bit)-1);

                bits >>= lpp_bit;
                int pi = bits & ((1<<npage_bit)-1);

                return reverse_bit(pi,npage_bit) | (reverse_bit(li,lpp_bit)<<npage_bit) | (reverse_bit(ci,cpl_bit) << (lpp_bit + npage_bit));
#else
                return reverse_bit(bits, npage_bit+lpp_bit+cpl_bit);
#endif
            };

            for (int i=0; i<nchain; i++) {
                int cur = shuf_bit(i);
                int next = 0;
                if (i == nchain-1) {
                    next = 0;
                } else {
                    next = shuf_bit(i+1);
                }

                random_ptr[cur].v = next;
            }

            std::vector<Chain> random_ptr2(random_ptr);

            for (int i = 0; i < nchain; i++) {
                int idx = random_ptr[i].v;
                random_ptr[i].v = (uintptr_t)&random_ptr[idx];
                random_ptr2[i].v = (uintptr_t)&random_ptr2[idx];
            }

            int start_depth = 4;

            for (int pi = 0; pi < (int)Buffer::NUM_BUFFER; pi++) {
                std::vector<double> history(max_depth);
                std::vector<double> d(max_depth);

                for (int di = start_depth; di < max_depth; di++) {
                    BufferEstimator ml((Buffer)pi, di, &random_ptr[0],
                                       &random_ptr2[0]);
                    ml.gen();

                    double v = ml.run(g);
                    history[di] = v;
                    //printf("%d,%f\n", di, v);
                }

                double min = 1e9;
                /* remove spike */
                for (int di=max_depth-1; di>=start_depth; di--) {
                    min = std::min(history[di], min);
                    history[di] = min;
                }

                int distance = 2;

                for (int di = start_depth + distance; di < max_depth; di++) {
                    d[di] = history[di] - history[di - distance];
                }

                int maxd_pos = 0;
                double maxd = 0;
                for (int di = start_depth + distance; di < max_depth; di++) {
                    if (d[di] > maxd) {
                        maxd = d[di];
                        maxd_pos = di;
                    }
                    // printf("%d:%f %f\n", di, history[di], d[di]);
                }

                labels.push_back(name_table[pi]);
                results.push_back(maxd_pos + BufferEstimator::load_chain_num);

                if (pi == 0) {
                    max_depth = maxd_pos + BufferEstimator::load_chain_num;
                }
            }
        }

        max_depth = std::min(256, max_depth);
        max_depth = std::max(8, max_depth);

        if (1) {
            int start_depth = 1;
            int distance = 2;
            double instr_ratio =
                (1.0 / SchedEstimator::dependency_length) * 1.5;

            for (bool multi : {true,false}) {
                for (int pi = 0; pi < (int)SchedPipe::NUM_PIPE; pi++) {
                    std::vector<double> history(max_depth);
                    for (int depth = start_depth; depth < max_depth; depth++) {
                        SchedEstimator se((SchedPipe)pi, depth, multi);
                        se.gen();
                        double v = se.run(g);
                        history[depth] = v;
                        //printf("%d:%f\n", depth, v);
                    }

                    double final_sum = 0;
                    for (int i = max_depth - 3; i < max_depth; i++) {
                        final_sum += history[i];
                    }

                    double final_value = final_sum / 3;

                    /* remove spike */
                    double minval = 1e9;
                    for (int depth=start_depth; depth < max_depth; depth++) {
                        minval = std::min(minval, history[depth]);
                        history[depth] = minval;
                    }

                    int find_pos = 0;

                    for (int depth = start_depth + distance; depth < max_depth;
                         depth++) {
                        double v = history[depth];
                        if ((v / final_value - 1.0) > instr_ratio) {
                            // printf("%d : %f %f %f OOO\n", depth, v,
                            // v/final_value, v-final_value);
                        } else {
                            // printf("%d : %f %f %f XXX\n", depth, v,
                            // v/final_value, v-final_value);
                            find_pos = depth;
                            break;
                        }
                    }
                    
                    if (multi) {
                        labels.push_back(std::string(pipe_name_table[pi]) +
                                         "(multi scheduler depth)");
                    } else {
                        labels.push_back(std::string(pipe_name_table[pi]) +
                                         "(single scheduler depth)");
                    }

                    if (find_pos == 3) {
                        printf("warning : scheduler depth is tiny, this processor's pipe may be in-order. The result is unreliable\n");
                    }

                    results.push_back(find_pos);
                }
            }
        }

        table_t *table = new table_t("test_name", results.size());
        table->row_label = labels;
        table->column_label = "result";

        for (size_t i = 0; i < results.size(); i++) {
            (*table)[i] = results[i];
        }

        return result_t(table);
    }

    int double_precision() { return 2; }

    bool available(const GlobalState *g) override {
        return g->has_ooo();
    }

    virtual result_t parse_json_result(picojson::value const &v) {
        return result_t(table_t::parse_json_result(v));
    }
};

} // namespace

std::unique_ptr<BenchDesc> get_pipe_desc() {
    return std::unique_ptr<BenchDesc>(new Pipe());
}

#else

std::unique_ptr<BenchDesc> get_pipe_desc() {
    return std::unique_ptr<BenchDesc>();
}

#endif

} // namespace smbm
