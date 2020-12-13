#include "sys-microbenchmark.h"
#include "table.h"
#include <unistd.h>
#include "oneshot_timer.h"

namespace smbm {

struct MemoryLatency : public BenchDesc {
    typedef Table1D<double, int> table_t;

    MemoryLatency() : BenchDesc("memory-random-access") {}

    virtual result_t run(GlobalState const *g) override {
        std::vector<int> range_cands;

        for (int x = 4; x < 512 * 1024; x *= 2) {
            range_cands.push_back(x);
        }

        table_t *result = new table_t("range[KiByte]", range_cands.size());
        result->column_label = "nsec/access";
        result->row_label = range_cands;

        return result_t(result);
    }

    virtual result_t parse_json_result(picojson::value const &v) {
        return result_t(table_t::parse_json_result(v));
    }
};

std::unique_ptr<BenchDesc> get_memory_random_access_desc() {
    return std::unique_ptr<BenchDesc>(new MemoryLatency());
}

} // namespace smbm