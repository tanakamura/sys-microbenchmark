#include "json.h"
#include "sys-microbenchmark.h"
#include "table.h"
#include <random>
#define _USE_MATH_DEFINES
#include "memalloc.h"
#include <math.h>

namespace smbm {

#ifdef HAVE_DYNAMIC_CODE_GENERATOR

namespace {

static inline void output4(char *&p, uint32_t v) {
    *(p++) = (v >> (8 * 0)) & 0xff;
    *(p++) = (v >> (8 * 1)) & 0xff;
    *(p++) = (v >> (8 * 2)) & 0xff;
    *(p++) = (v >> (8 * 3)) & 0xff;
}

struct Loop {
    typedef int (*loop_func_t)(const char *table, int loop);

    size_t inst_len;

    ExecutableMemory em;

    Loop(const Loop &r) = delete;
    Loop &operator=(const Loop &r) = delete;

    ~Loop() { free_executable(&em); }

#ifdef X86
    Loop(int ninsn, bool indirect_branch, int nindirect_brach_target) {
        uint32_t table_size = 0;
        size_t alloc_size = 0;
        size_t inst_size = 0;
        uint64_t *jmp_table = 0;

        if (indirect_branch) {
            inst_size = ninsn * ((12 + 3) + nindirect_brach_target) + 11 + 7;
            table_size = ninsn * nindirect_brach_target * 8;
            alloc_size = inst_size + table_size;
        } else {
            inst_size = ninsn * (7 + 3) + 11;
            alloc_size = inst_size;
        }

        this->em = alloc_exeutable(alloc_size);
        char *p = (char *)this->em.p;
        char *p0 = p;

        this->inst_len = inst_size;

        if (indirect_branch) {
            uint32_t table_delta = inst_size;

            /* lea r9, [rip + table_delta] */
            *(p++) = 0x4c; // 1
            *(p++) = 0x8d; // 2
            *(p++) = 0x0d; // 3

            output4(p, table_delta - 7); // 7
            jmp_table = (uint64_t *)(p0 + inst_size);
        }

#ifdef WINDOWS
        uint8_t first_argument = 0x1;  /* rcx */
        uint8_t second_argument = 0x2; /* rdx */
#else
        uint8_t first_argument = 0x7;  /* rdi */
        uint8_t second_argument = 0x6; /* rsi */
#endif

        *(p++) = 0x31; // 1
        *(p++) = 0xc0; /* 2 : xor eax, eax */

        char *loop_start = p;

        for (int i = 0; i < ninsn; i++) {
            /* if (*first_argument) ret++ */

            if (indirect_branch) {
                /* movzq r8, byte [first_argument] */
                *(p++) = 0x4c;           // 1
                *(p++) = 0x0f;           // 2
                *(p++) = 0xb6;           // 3
                *(p++) = first_argument; // 4

                uint32_t table_offset = i * nindirect_brach_target * 8;

                /* jmp [r9 + r8 *8 + table_offset] */
                *(p++) = 0x43;            // 5
                *(p++) = 0xff;            // 6
                *(p++) = 0xa4;            // 7
                *(p++) = 0xc1;            // 8
                output4(p, table_offset); // 12

                for (int ti = 0; ti < nindirect_brach_target; ti++) {
                    jmp_table[i * nindirect_brach_target + ti] = (uintptr_t)p;
                    *(p++) = 0x90; // nop
                }
            } else {
                /* cmp [first_argument], 0 */
                *(p++) = 0x80;                    // 1
                *(p++) = 0x38 | (first_argument); // 2
                *(p++) = 1;                       // 3

                /* jne +2 */
                *(p++) = 0x75; // 4
                *(p++) = 0x2;  // 5

                /* inc eax */
                *(p++) = 0xff; // 6
                *(p++) = 0xc0; // 7
            }

            /* inc first_argument */
            *(p++) = 0x48;                  // 1
            *(p++) = 0xff;                  // 2
            *(p++) = 0xc0 | first_argument; // 3
        }

        /* dec second_argument */
        *(p++) = 0xff;                   // 3
        *(p++) = 0xc8 | second_argument; // 4

        /* jnz loop_begin */
        uint64_t delta = (p - loop_start) + 6;
        uint32_t disp = -delta;

        *(p++) = 0x0f;    // 5
        *(p++) = 0x85;    // 6
        output4(p, disp); // 10

        *(p++) = 0xc3; // 11

        size_t actual_length = p - p0;
        if (actual_length != inst_size) {
            fprintf(stderr, "actual:%d alloc:%d\n", (int)actual_length,
                    (int)inst_size);
            // abort();
        }
    }
#else
#error "loop generator for this architecture is not yet implemented."
#endif

    void invoke(char const *table, int nloop) {
        ((loop_func_t)this->em.p)(table, nloop);
    }
};

struct ResultValue {
    double nsec;
    double cycle;
    double branch_miss;
};

enum class GenMethod { FULL, ITER, INST, COS };

static ResultValue run1(const GlobalState *g, int ninst, int nloop,
                        GenMethod gm, bool indirect,
                        int nindirect_brach_target) {
    perf_counter_value_t cycle0 = 0, branch0 = 0, cycle1 = 0, branch1 = 0;
    userland_timer_value t0, t1;
    struct ResultValue ret = {};

    std::vector<char> table(ninst * nloop);
    int rand_min = 0;
    int rand_max = 1;

    if (indirect) {
        rand_max = nindirect_brach_target - 1;
    }

    switch (gm) {
    case GenMethod::FULL: {
        std::mt19937 engine(0);
        std::uniform_int_distribution<> dist(rand_min, rand_max);

        for (int i = 0; i < ninst * nloop; i++) {
            int v = dist(engine);
            table[i] = v;
        }
    } break;

    case GenMethod::INST: {
        std::mt19937 engine(0);
        std::uniform_int_distribution<> dist(rand_min, rand_max);

        for (int i = 0; i < ninst; i++) {
            int v = dist(engine);

            for (int l = 0; l < nloop; l++) {
                table[l * ninst + i] = v;
            }
        }
    } break;

    case GenMethod::ITER: {
        std::mt19937 engine(0);
        std::uniform_int_distribution<> dist(rand_min, rand_max);

        for (int l = 0; l < nloop; l++) {
            int v = dist(engine);

            for (int i = 0; i < ninst; i++) {
                table[l * ninst + i] = v;
            }
        }
    } break;

    case GenMethod::COS: {
        for (int li = 0; li < nloop; li++) {
            for (int ii = 0; ii < ninst; ii++) {
                double ir = ii / ((double)ninst - 1.0);
                double max_lfreq = M_PI;
                double min_lfreq = M_PI / nloop;

                double dlfreq = max_lfreq - min_lfreq;
                double lfreq = min_lfreq + dlfreq * ir;

                double lv = cos(lfreq * li);

                if (lv < 0) {
                    table[li * ninst + ii] = 0;
                } else {
                    table[li * ninst + ii] = 1;
                }
            }
        }
    } break;
    }

    Loop loop(ninst, indirect, nindirect_brach_target);
    loop.invoke(&table[0], nloop);

    t0 = userland_timer_value::get();
    if (g->is_hw_perf_counter_available()) {
        cycle0 = g->get_hw_cpucycle();
        branch0 = g->get_hw_branch_miss();
    }

    int niter = 1024;
    for (int i = 0; i < niter; i++) {
        loop.invoke(&table[0], nloop);
    }
    t1 = userland_timer_value::get();

    if (g->is_hw_perf_counter_available()) {
        cycle1 = g->get_hw_cpucycle();
        branch1 = g->get_hw_branch_miss();
        ret.cycle = (cycle1 - cycle0) / (double)niter;
        ret.branch_miss = (branch1 - branch0) / (double)niter;
    }

    ret.nsec = (g->userland_timer_delta_to_sec(t1 - t0) / (double)niter) * 1e9;

    return ret;
}

struct RandomBranch : public BenchDesc {
    GenMethod gm;
    bool use_perf_counter;
    bool indirect;

    static const char *get_name(GenMethod m, bool use_perf_counter,
                                bool indirect) {
        if (indirect) {
            if (use_perf_counter) {
                return "random-indirect-branch-hit";

            } else {
                return "random-indirect-branch";
            }
        } else {
            if (use_perf_counter) {
                switch (m) {
                case GenMethod::FULL:
                    return "full-random-branch-hit";
                case GenMethod::ITER:
                    return "iter-random-branch-hit";
                case GenMethod::INST:
                    return "inst-random-branch-hit";
                case GenMethod::COS:
                    return "cos-branch-hit";
                }
            } else {
                switch (m) {
                case GenMethod::FULL:
                    return "full-random-branch";
                case GenMethod::ITER:
                    return "iter-random-branch";
                case GenMethod::INST:
                    return "inst-random-branch";
                case GenMethod::COS:
                    return "cos-branch";
                }
            }
        }
        return "";
    }

    RandomBranch(GenMethod m, bool use_perf_counter, bool indirect)
        : BenchDesc(RandomBranch::get_name(m, use_perf_counter, indirect)),
          gm(m), use_perf_counter(use_perf_counter), indirect(indirect) {}

    typedef Table1D<double, std::string> table_t;

    virtual result_t run(GlobalState const *g) override {
        table_t *ret;

        std::vector<int> count_table;
        std::vector<int> target_table;

        std::vector<std::string> row_label;
        int row = 0;

        if (indirect) {
            count_table = {4, 16, 32, 64, 128};
            target_table = {1,2,4};
        } else {
            count_table = {16, 32, 64, 128, 256, 512, 1024};
            target_table = {0};
        }

        ret = new table_t("table size (inst x nloop)", count_table.size() * target_table.size() * 3);

        if (this->use_perf_counter) {
            ret->column_label = "hit-rate[%]";
        } else {
            ret->column_label = "nsec/branch";
        }

        for (int ti : target_table) {
            for (int i : count_table) {
                auto add_label = [g, &row_label, ret, &row, this](int i0, int i1, int ti) {
                    auto r = run1(g, i0, i1, this->gm, this->indirect, ti);
                    char buf[512];
                    if (this->indirect) {
                        sprintf(buf, "%6d x %6d [nindirect_taregt=%d]", i0, i1, ti);
                    } else {
                        sprintf(buf, "%6d x %6d", i0, i1);
                    }
                    row_label.push_back(buf);

                    if (use_perf_counter) {
                        (*ret)[row++] =
                            (1.0 - (r.branch_miss / ((i0 + 1) * i1))) * 100;
                    } else {
                        (*ret)[row++] = r.nsec / ((i0 + 1) * i1);
                    }
                };

                add_label(i / 4, i * 4, ti);
                add_label(i, i, ti);
                add_label(i * 4, i / 4, ti);
            }
        }

        ret->row_label = row_label;

        return result_t(ret);
    }
    result_t parse_json_result(picojson::value const &v) override {
        return result_t(table_t::parse_json_result(v));
    }

    bool available(GlobalState const *g) override {
        if (this->use_perf_counter) {
            return g->is_hw_perf_counter_available();
        } else {
            return true;
        }
    }
};
} // namespace

std::unique_ptr<BenchDesc> get_random_branch_desc() {
    return std::unique_ptr<BenchDesc>(
        new RandomBranch(GenMethod::FULL, false, false));
}
std::unique_ptr<BenchDesc> get_inst_random_branch_desc() {
    return std::unique_ptr<BenchDesc>(
        new RandomBranch(GenMethod::INST, false, false));
}
std::unique_ptr<BenchDesc> get_iter_random_branch_desc() {
    return std::unique_ptr<BenchDesc>(
        new RandomBranch(GenMethod::ITER, false, false));
}
std::unique_ptr<BenchDesc> get_cos_branch_desc() {
    return std::unique_ptr<BenchDesc>(
        new RandomBranch(GenMethod::COS, false, false));
}

std::unique_ptr<BenchDesc> get_random_branch_hit_desc() {
    return std::unique_ptr<BenchDesc>(
        new RandomBranch(GenMethod::FULL, true, false));
}
std::unique_ptr<BenchDesc> get_inst_random_branch_hit_desc() {
    return std::unique_ptr<BenchDesc>(
        new RandomBranch(GenMethod::INST, true, false));
}
std::unique_ptr<BenchDesc> get_iter_random_branch_hit_desc() {
    return std::unique_ptr<BenchDesc>(
        new RandomBranch(GenMethod::ITER, true, false));
}
std::unique_ptr<BenchDesc> get_cos_branch_hit_desc() {
    return std::unique_ptr<BenchDesc>(
        new RandomBranch(GenMethod::COS, true, false));
}

std::unique_ptr<BenchDesc> get_indirect_branch_desc() {
    return std::unique_ptr<BenchDesc>(
        new RandomBranch(GenMethod::FULL, false, true));
}
std::unique_ptr<BenchDesc> get_indirect_branch_hit_desc() {
    return std::unique_ptr<BenchDesc>(
        new RandomBranch(GenMethod::FULL, true, true));
}

#else
std::unique_ptr<BenchDesc> get_random_branch_desc() {
    return std::unique_ptr<BenchDesc>();
}
std::unique_ptr<BenchDesc> get_inst_random_branch_desc() {
    return std::unique_ptr<BenchDesc>();
}
std::unique_ptr<BenchDesc> get_iter_random_branch_desc() {
    return std::unique_ptr<BenchDesc>();
}
std::unique_ptr<BenchDesc> get_cos_branch_desc() {
    return std::unique_ptr<BenchDesc>();
}

std::unique_ptr<BenchDesc> get_random_branch_hit_desc() {
    return std::unique_ptr<BenchDesc>();
}
std::unique_ptr<BenchDesc> get_inst_random_branch_hit_desc() {
    return std::unique_ptr<BenchDesc>();
}
std::unique_ptr<BenchDesc> get_iter_random_branch_hit_desc() {
    return std::unique_ptr<BenchDesc>();
}
std::unique_ptr<BenchDesc> get_cos_branch_hit_desc() {
    return std::unique_ptr<BenchDesc>();
}

std::unique_ptr<BenchDesc> get_indirect_branch_desc() {
    return std::unique_ptr<BenchDesc>();
}
std::unique_ptr<BenchDesc> get_indirect_branch_hit_desc() {
    return std::unique_ptr<BenchDesc>();
}


#endif

} // namespace smbm
