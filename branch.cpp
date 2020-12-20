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

using namespace json;

struct BranchResultEntry {
    typedef Table2D<char,int> tf_table_t;
    typedef Table1D<double,std::string> result_table_t;

    std::shared_ptr<tf_table_t> tf_table;
    std::shared_ptr<result_table_t> result_table;
    int cycle;
    double nsec;
    BranchResultEntry(int row, int col, bool has_perf_counter)
        :tf_table( new tf_table_t("instruction", "loop", row+1, col)),
         result_table( new result_table_t("result", has_perf_counter?3:1) )
    {
        std::vector<int> row_labels;
        for (int i=0; i<row+1; i++) {
            row_labels.push_back(i);
        }
        tf_table->row_label = row_labels;

        std::vector<int> col_labels;
        for (int i=0; i<col; i++) {
            col_labels.push_back(0);
        }
        tf_table->column_label = col_labels;

        std::vector<std::string> result_tag;
        result_tag.push_back("nsec");
        if (has_perf_counter) {
            result_tag.push_back("cycles");
            result_tag.push_back("branch-miss");
        }

        result_table->row_label = result_tag;
        result_table->column_label = "result";

        for (int l=0; l<col; l++) {
            if (l == col-1) {
                (*this->tf_table)[row][l] = 'N';
            } else {
                (*this->tf_table)[row][l] = 'T';
            }
        }
    }

    BranchResultEntry(picojson::value const &value) {
        auto obj = value.get<picojson::object>();

        auto tf = tf_table_t::parse_json_result(obj["tf"]);
        this->tf_table.reset(tf);

        auto r = result_table_t::parse_json_result(obj["result"]);
        this->result_table.reset(r);
    }

    picojson::value dump_json() const {
        picojson::object ret;

        ret["tf"] = tf_table->dump_json();
        ret["result"] = result_table->dump_json();

        return picojson::value(ret);
    }

    void dump_human_readable(std::ostream &os, int double_precision) const {
        tf_table->dump_human_readable(os, double_precision);
        result_table->dump_human_readable(os, double_precision);
    }
};


struct BranchResult
    :public BenchResult
{
    std::vector<BranchResultEntry> results;

    static std::unique_ptr<BenchResult> parse_json_result(picojson::value const &value) {
        auto obj = value.get<picojson::array>();
        std::unique_ptr<BranchResult> r(new BranchResult);

        for (auto && entry : obj) {
            r->results.emplace_back(entry);
        }

        return std::unique_ptr<BenchResult>(r.release());
    }

    picojson::value dump_json() const override {
        std::vector<picojson::value> ret;

        for (auto && entry : results) {
            ret.push_back(entry.dump_json());
        }

        return picojson::value(ret);
    }

    void dump_human_readable(std::ostream &os, int double_precision) override {
        for (auto && entry : results) {
            entry.dump_human_readable(os, double_precision);
        }
    }
};

struct BranchTable {
    std::vector<char> p;
    int ninst, nloop;

    BranchTable(int ninst, int nloop)
        :p(ninst*nloop), ninst(ninst), nloop(nloop)
    {
        memset(&p[0], 0, ninst*nloop);
    }

    ~BranchTable()
    {
    }
};

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

static void
set_tf(BranchResultEntry *e, const BranchTable *t)
{
    for (int inst=0; inst<t->ninst; inst++) {
        for (int loop=0; loop<t->nloop; loop++) {
            (*e->tf_table)[inst][loop] = t->p[loop*t->ninst + inst] ? 'T' : 'N';
        }
    }
}

static BranchResultEntry run1(const GlobalState *g, const BranchTable *t) {
    perf_counter_value_t cycle0=0, branch0=0, cycle1=0, branch1=0;
    userland_timer_value t0, t1;

    Loop loop(t->ninst);
    loop.invoke(&t->p[0], t->nloop);

    t0 = userland_timer_value::get();
    if (g->is_hw_perf_counter_available()) {
        cycle0 = g->get_hw_cpucycle();
        branch0 = g->get_hw_branch_miss();
    }

    int niter = 1024;
    for (int i=0; i<niter; i++) {
        loop.invoke(&t->p[0], t->nloop);
    }

    if (g->is_hw_perf_counter_available()) {
        cycle1 = g->get_hw_cpucycle();
        branch1 = g->get_hw_branch_miss();
        t1 = userland_timer_value::get();

        BranchResultEntry ret(t->ninst, t->nloop, true);

        (*ret.result_table)[0] = (g->userland_timer_delta_to_sec(t1-t0)/(double)niter)*1e9;
        (*ret.result_table)[1] = (cycle1 - cycle0)/(double)niter;
        (*ret.result_table)[2] = (branch1 - branch0)/(double)niter;

        set_tf(&ret, t);
        return ret;
    } else {
        t1 = userland_timer_value::get();
        BranchResultEntry ret(t->ninst, t->nloop, false);
        (*ret.result_table)[0] = g->userland_timer_delta_to_sec(t1-t0)/(double)niter;
        set_tf(&ret, t);
        return ret;
    }
}

static void all_true16(BranchTable &t) {
    int ninst=16;
    int nloop=16;

    t = BranchTable(ninst, nloop);

    for (int i=0; i<ninst*nloop; i++) {
        t.p[i] = 1;
    }
}

static void all_true64(BranchTable &t) {
    int ninst=64;
    int nloop=64;

    t = BranchTable(ninst, nloop);

    for (int i=0; i<ninst*nloop; i++) {
        t.p[i] = 1;
    }
}

static void rand16(BranchTable &t) {
    int ninst=16;
    int nloop=16;

    std::mt19937 engine(0);
    std::uniform_int_distribution<> dist(0,1);

    t = BranchTable(ninst, nloop);

    for (int i=0; i<ninst*nloop; i++) {
        int v = dist(engine);
        t.p[i] = v;
    }
}

static void rand32(BranchTable &t) {
    int ninst=32;
    int nloop=32;

    std::mt19937 engine(0);
    std::uniform_int_distribution<> dist(0,1);

    t = BranchTable(ninst, nloop);

    for (int i=0; i<ninst*nloop; i++) {
        int v = dist(engine);
        t.p[i] = v;
    }
}

static void rand64(BranchTable &t) {
    int ninst=64;
    int nloop=64;

    std::mt19937 engine(0);
    std::uniform_int_distribution<> dist(0,1);

    t = BranchTable(ninst, nloop);

    for (int i=0; i<ninst*nloop; i++) {
        int v = dist(engine);
        t.p[i] = v;
    }
}

static void all_true256x128(BranchTable &t) {
    int ninst=256;
    int nloop=128;

    t = BranchTable(ninst, nloop);

    for (int i=0; i<ninst*nloop; i++) {
        t.p[i] = 1;
    }
}

static void rand256x128(BranchTable &t) {
    int ninst=256;
    int nloop=128;

    std::mt19937 engine(0);
    std::uniform_int_distribution<> dist(0,1);

    t = BranchTable(ninst, nloop);

    for (int i=0; i<ninst*nloop; i++) {
        int v = dist(engine);
        t.p[i] = v;
    }
}

static void all_true256(BranchTable &t) {
    int ninst=256;
    int nloop=256;

    t = BranchTable(ninst, nloop);

    for (int i=0; i<ninst*nloop; i++) {
        t.p[i] = 1;
    }
}

static void rand256(BranchTable &t) {
    int ninst=256;
    int nloop=256;

    std::mt19937 engine(0);
    std::uniform_int_distribution<> dist(0,1);

    t = BranchTable(ninst, nloop);

    for (int i=0; i<ninst*nloop; i++) {
        int v = dist(engine);
        t.p[i] = v;
    }
}


#define FOR_EACH_TEST(F)                        \
    F(all_true16)                                 \
    F(rand16)                                   \
    F(rand32)                                   \
    F(all_true64)                               \
    F(rand64)                                   \
    F(all_true256x128)                               \
    F(rand256x128)                                   \
    F(all_true256)                                   \
    F(rand256)                                   \

struct Branch : public BenchDesc {
    Branch()
        :BenchDesc("branch")
    {}

    virtual result_t run(GlobalState const *g) override {
        auto ret = std::make_unique<BranchResult>();
        BranchTable bt(1,1);

#define RUN1(t) { t(bt); ret->results.push_back(run1(g,&bt)); }

        FOR_EACH_TEST(RUN1)

        return result_t(ret.release());
    }
    virtual result_t parse_json_result(picojson::value const &v) {
        return result_t(BranchResult::parse_json_result(v));
    }

};
}


std::unique_ptr<BenchDesc> get_branch_desc() {
    return std::unique_ptr<BenchDesc>(new Branch());
}


}
