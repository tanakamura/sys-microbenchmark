#include <fstream>
#include <iomanip>
#include "sys-microbenchmark.h"

using namespace smbm;

int
main(int argc, char **argv)
{
    auto blist = get_all_benchmark_list();

    for (int i=1; i<argc; i++) {
        std::ifstream ifs;
        ifs.open(argv[i]);
        if (!ifs) {
            perror(argv[1]);
            return 1;
        }

        picojson::value v;
        ifs >> v;

        ResultListSet rls;

        deserialize_result(rls, v);

        for (size_t i=0; i<rls.lists.size(); i++) {
            auto &sysinfo = rls.lists[i].sysinfo;
            auto &results = rls.lists[i].results;

            printf("result[%d] : OS : %s\n", (int)i, sysinfo.os.c_str());
            printf("result[%d] : CPU : %s\n", (int)i, sysinfo.cpuid.c_str());

            for (auto &b : blist) {
                auto it = results.find(b->name);
                if (it != results.end()) {
                    std::cout << std::fixed << std::setprecision(b->double_precision());
                    it->second->dump_human_readable(std::cout, b->double_precision());
                }
            }
        }
    }
}
