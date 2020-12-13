#include "oneshot_timer.h"
#include "sys-microbenchmark.h"
#include "table.h"
#include <unistd.h>
#include <algorithm>
#include <random>

namespace smbm {

struct MemoryLatency : public BenchDesc {
    typedef Table1D<double, int> table_t;

    bool has_dep;

    MemoryLatency(bool has_dep)
        : BenchDesc(has_dep ? "random-access-seq"
                            : "random-access-para"),
          has_dep(has_dep) {}

    virtual result_t run(GlobalState const *g) override {
        std::vector<int> range_cands;
        std::vector<int> range_label;

        for (int x = 128; x < 64 * 1024 * 1024; x *= 2) {
            range_cands.push_back(x);
            range_label.push_back(x * sizeof(int));
        }

        table_t *result = new table_t("range[KiByte]", range_cands.size());
        result->column_label = "nsec/access";
        result->row_label = range_label;

        for (int i=0; i<(int)range_cands.size(); i++) {
            int x = range_cands[i];
            std::vector<int> ptr(x);
            std::vector<int> v100(x, 100);

            for (int i=0; i<x; i++) {
                ptr[i] = i;
            }

            std::mt19937 mt;
            mt.seed(0);
            std::shuffle(ptr.begin(), ptr.end(), mt);

            int sum = g->getzero();
            int count = 0;

            oneshot_timer ot(1024);
            ot.start(g, 0.2);
            if (has_dep) {
                int pos = 0;
                int cur = 0;
                int first = ptr[cur];
                while (! ot.test_end()) {
                    int next = ptr[cur];
                    sum += next;
                    if (next == first) { // break ring
                        pos = (pos+1) % x;
                        first = ptr[pos];
                        cur = ptr[first];
                    } else {
                        cur = ptr[cur];
                    }
                    count++;
                }
            } else {
                int pos = 0;
                while (! ot.test_end()) {
                    sum += v100[ptr[pos]];
                    pos = (pos+1) % x;
                    count++;
                }
            }
            g->dummy_write(0, sum);

            double sec = ot.actual_interval_sec(g);

            result->v[i] = (sec*1e9) / count;
        }


        return result_t(result);
    }

    virtual result_t parse_json_result(picojson::value const &v) {
        return result_t(table_t::parse_json_result(v));
    }
};

std::unique_ptr<BenchDesc> get_memory_random_access_seq_desc() {
    return std::unique_ptr<BenchDesc>(new MemoryLatency(true));
}
std::unique_ptr<BenchDesc> get_memory_random_access_para_desc() {
    return std::unique_ptr<BenchDesc>(new MemoryLatency(false));
}

} // namespace smbm