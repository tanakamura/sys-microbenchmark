#include <random>
#include "sys-microbenchmark.h"
#include "table.h"
#include "json.h"

#ifdef POSIX
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace smbm {

namespace {

struct Loop {
    typedef int (*loop_func_t) (const char *table, int loop);

    size_t buffer_len;
    size_t inst_len;
    char *memory;

    Loop(const Loop &r) = delete;
    Loop & operator = (const Loop &r) = delete;

    ~Loop() {
        munmap(memory, buffer_len);
    }

#ifdef X86
    Loop(int ninsn) {
        size_t alloc_size = 10*ninsn + 11;
        size_t map_size = (alloc_size + 4095) & ~4095;

        char *p0 = (char*)mmap(0, map_size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);
        char *p = p0;

        if (p0 == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }

        this->buffer_len  = map_size;
        this->inst_len = alloc_size;
        this->memory = p0;

#ifdef WINDOWS
        uint8_t first_argument = 0x1; /* rcx */
        uint8_t second_argument = 0x2; /* rdx */
#else
        uint8_t first_argument = 0x7; /* rdi */
        uint8_t second_argument = 0x6; /* rsi */
#endif

        *(p++) = 0x31;
        *(p++) = 0xc0;              /* xor eax, eax */

        char *loop_start = p;

        for (int i=0; i<ninsn; i++) {
            /* if (*first_argument) ret++ */

            /* cmp [first_argument], 0 */
            *(p++) = 0x80;
            *(p++) = 0x38 | (first_argument);
            *(p++) = 1;

            /* jne +2 */
            *(p++) = 0x75;
            *(p++) = 0x2;

            /* inc eax */
            *(p++) = 0xff;
            *(p++) = 0xc0;

            /* inc first_argument */
            *(p++) = 0x48;
            *(p++) = 0xff;
            *(p++) = 0xc0 | first_argument;
        }

        /* dec second_argument */
        *(p++) = 0xff;
        *(p++) = 0xc8 | second_argument;

        /* jnz loop_begin */
        uint64_t delta = (p - loop_start) + 6;
        uint32_t disp = -delta;

        *(p++) = 0x0f;
        *(p++) = 0x85;
        *(p++) = (disp>>(8*0)) & 0xff;
        *(p++) = (disp>>(8*1)) & 0xff;
        *(p++) = (disp>>(8*2)) & 0xff;
        *(p++) = (disp>>(8*3)) & 0xff;

        *(p++) = 0xc3;

        size_t actual_length = p - p0;
        if (actual_length != alloc_size) {
            fprintf(stderr, "actual:%d alloc:%d\n", (int)actual_length, (int)alloc_size);
            abort();
        }
    }
#else
#error "loop generator for this architecture is not yet implemented."
#endif

    void invoke(char const *table, int nloop) {
        ((loop_func_t)this->memory)(table, nloop);
    }
};

struct ResultValue {
    double nsec;
    double cycle;
    double branch_miss;
};

static ResultValue run1(const GlobalState *g, int ninst, int nloop) {
    perf_counter_value_t cycle0=0, branch0=0, cycle1=0, branch1=0;
    userland_timer_value t0, t1;
    struct ResultValue ret = {};

    std::vector<char> table(ninst * nloop);

    std::mt19937 engine(0);
    std::uniform_int_distribution<> dist(0,1);

    for (int i=0; i<ninst*nloop; i++) {
        int v = dist(engine);
        table[i] = v;
    }

    Loop loop(ninst);
    loop.invoke(&table[0], nloop);

    t0 = userland_timer_value::get();
    if (g->is_hw_perf_counter_available()) {
        cycle0 = g->get_hw_cpucycle();
        branch0 = g->get_hw_branch_miss();
    }

    int niter = 1024;
    for (int i=0; i<niter; i++) {
        loop.invoke(&table[0], nloop);
    }
    t1 = userland_timer_value::get();

    if (g->is_hw_perf_counter_available()) {
        cycle1 = g->get_hw_cpucycle();
        branch1 = g->get_hw_branch_miss();
        ret.cycle = (cycle1 - cycle0)/(double)niter;
        ret.branch_miss = (branch1 - branch0)/(double)niter;
    }

    ret.nsec = (g->userland_timer_delta_to_sec(t1-t0)/(double)niter)*1e9;

    return ret;
}

struct RandomBranch : public BenchDesc {
    RandomBranch()
        :BenchDesc("random-branch")
    {}

    typedef Table1D<double,std::string> table_t;

    virtual result_t run(GlobalState const *g) override {
        table_t *ret;

        std::vector<int> count_table = {16, 32, 64, 128, 256, 512};
        std::vector<std::string> row_label;
        int row=0;

        ret = new table_t("table size (inst x nloop)", count_table.size() * 3);
        ret->column_label = "nsec/branch";

        for (int i : count_table) {
            auto add_label = [g,&row_label,ret,&row](int i0, int i1) {
                auto r = run1(g, i0, i1);
                char buf[512];
                sprintf(buf, "%6d x %6d", i0, i1);
                row_label.push_back(buf);
                (*ret)[row++] = r.nsec / ((i0+1)*i1);
            };

            add_label(i/2, i*2);
            add_label(i, i);
            add_label(i*2, i/2);
        }

        ret->row_label = row_label;

        return result_t(ret);
    }
    virtual result_t parse_json_result(picojson::value const &v) {
        return result_t(table_t::parse_json_result(v));
    }

};
}


std::unique_ptr<BenchDesc> get_random_branch_desc() {
    return std::unique_ptr<BenchDesc>(new RandomBranch());
}


}
