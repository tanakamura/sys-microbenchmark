#include "sys-microbenchmark.h"
#include <algorithm>
#include "table.h"

namespace smbm {

static std::unique_ptr<BenchResult> run(std::ostream &o) {
    bool bit64 = false;

    int n_divider_bit = 63;
    int n_divisor_bit = 63;

    if (!bit64) {
        n_divider_bit = 32;
        n_divisor_bit = 33;
    }

    typedef Table2D<uint32_t,uint32_t> result_t;
    result_t*result_table(new result_t("divisor_bit", "divider_bit", n_divisor_bit, n_divider_bit));

    uint32_t max = 0;

    for (int divisor_bit = 0; divisor_bit <= n_divisor_bit; divisor_bit++) {
        result_table->row_label[divisor_bit] = divisor_bit;
    }
    for (int divider_bit = 0; divider_bit <= n_divider_bit; divider_bit++) {
        result_table->column_label[divider_bit-1] = divider_bit;
    }

    perf_counter pc;

    for (int divisor_bit = 0; divisor_bit <= n_divisor_bit; divisor_bit++) {
        for (int divider_bit = 1; divider_bit <= n_divider_bit; divider_bit++) {
            uint64_t divisor = (uint64_t)((1ULL << divisor_bit) - 1);
            uint64_t divider = (uint64_t)((1ULL << divider_bit) - 1);

            uint64_t t0 = pc.cpu_cycles();

            if (bit64) {
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

    dump2d(o, result_table, false);

    return std::unique_ptr<BenchResult>(result_table);
}

BenchDesc get_idiv_desc() {
    BenchDesc ret;
    ret.name = "idiv";
    ret.run = run;

    return ret;
}

} // namespace smbm
