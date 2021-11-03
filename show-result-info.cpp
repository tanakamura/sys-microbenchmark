#include "sys-microbenchmark.h"
#include <fstream>
#include <getopt.h>
#include <unistd.h>

using namespace smbm;

enum {
    ARG_COMPARE = 256,
    ARG_INPUT,
};

int main(int argc, char **argv) {
    std::string path;
    bool compare = false;

    while (1) {
        int option_index = 0;

        static struct option long_options[] = {
            {"compare", no_argument, 0, ARG_COMPARE},
            {"input", required_argument, 0, ARG_INPUT},
            {0, 0, 0, 0}};

        int c = getopt_long(argc, argv, "", long_options, &option_index);

        switch (c) {
        case ARG_COMPARE:
            compare = true;
            break;

        case ARG_INPUT:
            path = optarg;
            break;

        default:
            puts("usage: show-result-info --input src [--compare]");
            return 1;
        }
    }

    if (path == "") {
        puts("usage: show-result-info --input src [--compare]");
        return 1;
    }

    ResultListSet rls;

    std::ifstream ifs;
    ifs.open(path);
    if (!ifs) {
        perror(path.c_str());
        return 1;
    }
    picojson::value v;
    ifs >> v;
    ResultListSet src;
    deserialize_result(src, v);

    auto bench_list = get_all_benchmark_list();

    if (compare) {
        
    } else {
        int i = 0;
        for (auto &&r : src.lists) {
            std::cout << "result[" << i << "]::"
                      << "os :" << r.sysinfo.os << "\n";
            std::cout << "result[" << i << "]::"
                      << "date :" << r.sysinfo.date << "\n";
            std::cout << "result[" << i << "]::"
                      << "cpu :" << r.sysinfo.cpuid << "\n";
            i++;

            auto &results = r.results;

            for (auto &&b : bench_list) {
                auto it = results.find(b->name);
                if (it == results.end()) {
                    continue;
                }

                it->second->dump_human_readable(std::cout,
                                                b->double_precision());
            }
        }
    }

    return 0;
}
