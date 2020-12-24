#include "sys-microbenchmark.h"
#include "table.h"
#include "memalloc.h"

namespace smbm {
namespace {

struct Inst {
    int operands[2];

    virtual int size() const = 0;
    virtual void gen(char *&p) const = 0;

    virtual ~Inst() {}
};

} // namespace
} // namespace smbm

#ifdef X86
#include "pipe-x86.h"
#else

#endif

namespace smbm {

namespace {

static double measure_latency() { return 0; }

struct Pipe : public BenchDesc {
    Pipe() : BenchDesc("pipe") {}

    typedef Table1D<double, std::string> table_t;

    virtual result_t run(GlobalState const *g) override {
        table_t *table = new table_t("test_name", 0);

        ExecutableMemory m = alloc_exeutable(2048);

        char *p = (char*)m.p;
        enter_frame(p, callee_saved_int_regs, callee_saved_fp_regs);
        exit_frame(p);

        typedef void (*fp_t)(void);
        fp_t func = (fp_t)m.p;
        func();

        free_executable(&m);

        return result_t(table);
    }

    bool available(const GlobalState *g) override {
#ifdef X86
        return true;
#else
        return false;
#endif
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
