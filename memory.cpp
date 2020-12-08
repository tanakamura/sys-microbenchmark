#include "sys-microbenchmark.h"
#include "table.h"
#include <unistd.h>
#include "oneshot_timer.h"

namespace smbm {

struct MemoryBandwidth : public BenchDesc {
    typedef Table1D<double, int> table_t;

    MemoryBandwidth() : BenchDesc("memory-bandwidth") {}

    virtual result_t run(GlobalState *g) override {
        int ncore = sysconf(_SC_NPROCESSORS_ONLN);
        std::vector<int> cores;

        for (int i = 1; i < ncore; i *= 2) {
            cores.push_back(i);
        }

        table_t *result = new table_t("threads", cores.size());
        result->column_label = "MB/sec";
        result->row_label = cores;

        return result_t(result);
    }

    virtual result_t parse_json_result(picojson::value const &v) {
        return result_t(table_t::parse_json_result(v));
    }
};

std::unique_ptr<BenchDesc> get_memory_bandwidth_desc() {
    return std::unique_ptr<BenchDesc>(new MemoryBandwidth());
}

struct MemoryLatency : public BenchDesc {
    typedef Table1D<double, int> table_t;

    MemoryLatency() : BenchDesc("memory-latency") {}

    virtual result_t run(GlobalState *g) override {
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

std::unique_ptr<BenchDesc> get_memory_latency_desc() {
    return std::unique_ptr<BenchDesc>(new MemoryLatency());
}

} // namespace smbm