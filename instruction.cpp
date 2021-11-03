#include "cpu-feature.h"
#include "memalloc.h"
#include "oneshot_timer.h"
#include "simple-run.h"
#include "sys-features.h"
#include "sys-microbenchmark.h"
#include "table.h"

namespace smbm {

namespace {

#ifdef X86

#include "instruction-x86.h"

#elif defined AARCH64

#include "instruction-aarch64.h"

#else

#define FOR_EACH_TEST(F)

#endif

typedef Table1DBenchDesc<double, std::string> parent_t;

struct Instruction : public parent_t {
    Instruction() : parent_t("instruction") {}

    virtual result_t run(GlobalState const *g) override {
        int count = 0;
#define INC_COUNT(F, T)                                                        \
    if (T()) {                                                                 \
        count++;                                                               \
    }

        FOR_EACH_TEST(INC_COUNT);

        table_t *result = new table_t("test_name", count);

#define NAME(F, T)                                                             \
    if (T()) {                                                                 \
        labels.push_back(#F);                                                  \
    }

        result->column_label = " nsec/instruction";
        std::vector<std::string> labels;
        FOR_EACH_TEST(NAME);
        result->row_label = labels;

        count = 0;

#define RUN(F, T)                                                              \
    if (T()) {                                                                 \
        F t;                                                                   \
        (*result)[count++] = run_test_unroll32(g, &t);                         \
    }
        FOR_EACH_TEST(RUN)

        return std::unique_ptr<BenchResult>(result);
    }
    bool available(GlobalState const *g) override {
#if (defined X86) || (defined AARCH64)
        return true;
#else
        return false;
#endif
    }
};

} // namespace

std::unique_ptr<BenchDesc> get_instructions_desc() {
    return std::unique_ptr<BenchDesc>(new Instruction());
}

} // namespace smbm
