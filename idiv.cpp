#include "sys-microbenchmark.h"
#include "table.h"
#include <algorithm>

namespace smbm {

template <bool use_perf_counter> auto get_count(GlobalState const *g);
template <bool use_perf_counter>
auto convert_deltaval(GlobalState const *g, uint64_t v);

#ifdef HAVE_HW_PERF_COUNTER
template <> auto get_count<true>(GlobalState const *g) {
    return g->get_hw_cpucycle();
}

template <> auto convert_deltaval<true>(GlobalState const *g, uint64_t v) {
    return v;
}

#endif

template <> auto get_count<false>(GlobalState const *g) {
    return userland_timer_value::get();
}
template <> auto convert_deltaval<false>(GlobalState const *g, uint64_t d) {
    return g->userland_timer_delta_to_sec(d) * 1e9; // to nsec
}

typedef Table2DBenchDesc<double, uint32_t> parent_t;

template <typename int_t, bool is_32, bool use_perf_counter>
std::unique_ptr<BenchResult> run(GlobalState const *g) {
    int n_divider_bit = 63;
    int n_divisor_bit = 63;

    if (is_32) {
        n_divider_bit = 32;
        n_divisor_bit = 33;
    }

    parent_t::table_t *result_table(new parent_t::table_t(
        "divisor_bit", "divider_bit", n_divisor_bit + 1, n_divider_bit));

    for (int divisor_bit = 0; divisor_bit <= n_divisor_bit; divisor_bit++) {
        result_table->row_label[divisor_bit] = divisor_bit;
    }
    for (int divider_bit = 1; divider_bit <= n_divider_bit; divider_bit++) {
        result_table->column_label[divider_bit - 1] = divider_bit;
    }

    for (int divisor_bit = 0; divisor_bit <= n_divisor_bit; divisor_bit++) {
        for (int divider_bit = 1; divider_bit <= n_divider_bit; divider_bit++) {
            int_t divisor = (int_t)((1ULL << divisor_bit) - 1);
            int_t divider = (int_t)((1ULL << divider_bit) - 1);
            int_t zero = g->getzero();

            auto t0 = get_count<use_perf_counter>(g);
            int nloop = 2048;

            for (int i = 0; i < nloop; i++) {
                int_t x;

                REP16(x = divisor / divider; x &= zero; divisor |= x;);
            }
            auto t1 = get_count<use_perf_counter>(g);

            g->dummy_write(0, divisor);

            auto deltaval = convert_deltaval<use_perf_counter>(g, t1 - t0);

            double result = deltaval / (nloop * 16.0);

            (*result_table)[divisor_bit][divider_bit - 1] = result;
        }
    }

    return std::unique_ptr<BenchResult>(result_table);
}

template <bool use_perf_counter, bool static_available>
struct IDIV : public parent_t {
    bool is_32;
    IDIV(bool is_32)
        : parent_t(use_perf_counter
                   ? (is_32 ? "idiv32-cycle" : "idiv64-cycle")
                   : (is_32 ? "idiv32-realtime" : "idiv64-realtime"),
                   LOWER_IS_BETTER),
          is_32(is_32) {}

    result_t run(GlobalState const *g) override {
        if (is_32) {
            return smbm::run < uint32_t, true,
                   use_perf_counter && static_available > (g);
        } else {
            return smbm::run < uint64_t, false,
                   use_perf_counter && static_available > (g);
        }
    }

    int double_precision() override { return 1; }

    bool available(const GlobalState *g) override {
        if (!static_available) {
            return false;
        }

        if (use_perf_counter) {
            return g->is_hw_perf_counter_available();
        } else {
            return true;
        }
    }
};

std::unique_ptr<BenchDesc> get_idiv32_desc() {
    return std::unique_ptr<BenchDesc>(new IDIV<false, true>(true));
}
std::unique_ptr<BenchDesc> get_idiv64_desc() {
    return std::unique_ptr<BenchDesc>(new IDIV<false, true>(false));
}

#ifdef HAVE_HW_PERF_COUNTER
std::unique_ptr<BenchDesc> get_idiv32_cycle_desc() {
    return std::unique_ptr<BenchDesc>(new IDIV<true, true>(true));
}
std::unique_ptr<BenchDesc> get_idiv64_cycle_desc() {
    return std::unique_ptr<BenchDesc>(new IDIV<true, true>(false));
}
#else
std::unique_ptr<BenchDesc> get_idiv32_cycle_desc() {
    return std::unique_ptr<BenchDesc>(new IDIV<true, false>(true));
}
std::unique_ptr<BenchDesc> get_idiv64_cycle_desc() {
    return std::unique_ptr<BenchDesc>(new IDIV<true, false>(false));
}
#endif

} // namespace smbm
