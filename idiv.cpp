#include "sys-microbenchmark.h"
#include <algorithm>
#include "table.h"

namespace smbm {

static std::unique_ptr<BenchResult> run(bool is_32) {
    int n_divider_bit = 63;
    int n_divisor_bit = 63;

    if (is_32) {
        n_divider_bit = 32;
        n_divisor_bit = 33;
    }

    typedef Table2D<uint32_t,uint32_t> result_t;
    result_t*result_table(new result_t("divisor_bit", "divider_bit", n_divisor_bit+1, n_divider_bit));

    uint32_t max = 0;

    for (int divisor_bit = 0; divisor_bit <= n_divisor_bit; divisor_bit++) {
        result_table->row_label[divisor_bit] = divisor_bit;
    }
    for (int divider_bit = 1; divider_bit <= n_divider_bit; divider_bit++) {
        result_table->column_label[divider_bit-1] = divider_bit;
    }

    perf_counter pc;

    for (int divisor_bit = 0; divisor_bit <= n_divisor_bit; divisor_bit++) {
        for (int divider_bit = 1; divider_bit <= n_divider_bit; divider_bit++) {
            uint64_t divisor = (uint64_t)((1ULL << divisor_bit) - 1);
            uint64_t divider = (uint64_t)((1ULL << divider_bit) - 1);

            uint64_t t0 = pc.cpu_cycles();

            if (!is_32) {
                uint64_t dx = 0;
                uint64_t ax = divisor;

                for (int i=0; i<1024; i++) {
                    asm volatile (
                        REP16(
                            "divq %[divider]\n\t"
                            "andq %[zero], %[ax]\n\t"
                            "andq %[zero], %[dx]\n\t"
                            "orq %[divisor], %[ax]\n\t"
                            )

                        :[ax]"+a"(ax),
                         [dx]"+d"(dx)
                        :[divider]"r"(divider),
                         [divisor]"r"(divisor),
                         [zero]"r"(0ULL)
                        :"memory"
                        );
                }
            } else {
                uint32_t dx = 0;
                uint32_t ax = divisor;

                for (int i=0; i<1024; i++) {
                    asm volatile (
                        REP16(
                            "div %[divider]\n\t"
                            "and %[zero], %[ax]\n\t"
                            "and %[zero], %[dx]\n\t"
                            "or %[divisor], %[ax]\n\t"
                            )

                        :[ax]"+a"(ax),
                         [dx]"+d"(dx)
                        :[divider]"r"((uint32_t)divider),
                         [divisor]"r"((uint32_t)divisor),
                         [zero]"r"(0)
                        :"memory"
                        );
                }
            }

            uint64_t t1 = pc.cpu_cycles();

            uint32_t result = (t1-t0) / (1024 * 16);
            //printf("%d %d\n", (int)divisor, (int)divider);
            //uint32_t result = divisor / divider;

            (*result_table)[divisor_bit][divider_bit - 1] = result;
            max = std::max(max, result);
        }
    }

    return std::unique_ptr<BenchResult>(result_table);
}

struct IDIV
    :public BenchDesc
{
    bool is_32;
    IDIV(bool is_32)
        :BenchDesc(is_32?"idiv32":"idiv64"),
         is_32(is_32)
    {
    }

    result_t run() override {
        return smbm::run(is_32);
    }

    result_t parse_json_result(picojson::value const &v) override {
        return nullptr;
    }
};

std::unique_ptr<BenchDesc> get_idiv32_desc() {
    return std::unique_ptr<BenchDesc> (new IDIV(true));
}
std::unique_ptr<BenchDesc> get_idiv64_desc() {
    return std::unique_ptr<BenchDesc> (new IDIV(false));
}

} // namespace smbm
