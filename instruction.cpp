#include "cpu-feature.h"
#include "memalloc.h"
#include "oneshot_timer.h"
#include "simple-run.h"
#include "sys-microbenchmark.h"
#include "table.h"

namespace smbm {

namespace {

#ifdef X86

#include "instruction-x86.h"

#elif defined AARCH64

#include "instruction-aarch64.h"

#else

#define FOR_EACH_TEST(F)                        \

#endif

struct Instruction : public BenchDesc {
    Instruction() : BenchDesc("instruction") {}

    virtual result_t run(GlobalState const *g) override {
        int count = 0;
#define INC_COUNT(F, T)                                                        \
    if (T()) {                                                                 \
        count++;                                                               \
    }

        FOR_EACH_TEST(INC_COUNT);

        typedef Table1D<double, std::string> result_t;
        result_t *result = new result_t("test_name", count);

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
    result_t parse_json_result(picojson::value const &v) override {
        return result_t(Table1D<double, std::string>::parse_json_result(v));
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
