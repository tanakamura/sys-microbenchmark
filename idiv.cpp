#include "sys-microbenchmark.h"
#include <algorithm>
#include "table.h"

namespace smbm {

template<typename int_t, bool is_32>
std::unique_ptr<BenchResult> run(GlobalState *g) {
    int n_divider_bit = 63;
    int n_divisor_bit = 63;

    if (is_32) {
        n_divider_bit = 32;
        n_divisor_bit = 33;
    }

    typedef Table2D<double,uint32_t> result_t;
    result_t*result_table(new result_t("divisor_bit", "divider_bit", n_divisor_bit+1, n_divider_bit));

    double max = 0;

    for (int divisor_bit = 0; divisor_bit <= n_divisor_bit; divisor_bit++) {
        result_table->row_label[divisor_bit] = divisor_bit;
    }
    for (int divider_bit = 1; divider_bit <= n_divider_bit; divider_bit++) {
        result_table->column_label[divider_bit-1] = divider_bit;
    }

    for (int divisor_bit = 0; divisor_bit <= n_divisor_bit; divisor_bit++) {
        for (int divider_bit = 1; divider_bit <= n_divider_bit; divider_bit++) {
            int_t divisor = (int_t)((1ULL << divisor_bit) - 1);
            int_t divider = (int_t)((1ULL << divider_bit) - 1);
            int_t zero = 0;
            asm volatile (" " : "+r"(zero));

            auto t0 = g->get_cputime();
            int nloop = 2048;

            for (int i=0; i<nloop; i++) {
                int_t x;

                REP16(
                    x = divisor/divider;
                    x &= zero;
                    divisor |= x;
                    );
            }
            asm volatile (" " : : "r"(divisor));
            auto t1 = g->get_cputime();

            double result = g->delta_cputime(&t1,&t0) / (nloop * 16);
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

    result_t run(GlobalState *g) override {
        if (is_32) {
            return smbm::run<uint32_t,true>(g);
        } else {
            return smbm::run<uint64_t,false>(g);
        }
    }

    result_t parse_json_result(picojson::value const &v) override {
        return result_t(Table2D<uint32_t,uint32_t>::parse_json_result(v));
    }

    int double_precision() override {
        return 1;
    }
};

std::unique_ptr<BenchDesc> get_idiv32_desc() {
    return std::unique_ptr<BenchDesc> (new IDIV(true));
}
std::unique_ptr<BenchDesc> get_idiv64_desc() {
    return std::unique_ptr<BenchDesc> (new IDIV(false));
}

} // namespace smbm
