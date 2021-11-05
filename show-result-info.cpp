#include "sys-microbenchmark.h"
#include <fstream>
#include <getopt.h>
#include <unistd.h>

using namespace smbm;

enum { ARG_COMPARE = 256, ARG_INPUT, ARG_BASE, ARG_TEST_NAME };

int main(int argc, char **argv) {
    std::string path;
    std::string test_name;

    bool compare = false;
    int base = 0;

    while (1) {
        int option_index = 0;

        static struct option long_options[] = {
            {"compare", no_argument, 0, ARG_COMPARE},
            {"input", required_argument, 0, ARG_INPUT},
            {"base", required_argument, 0, ARG_BASE},
            {"test-name", required_argument, 0, ARG_TEST_NAME},
            {0, 0, 0, 0}};

        int c = getopt_long(argc, argv, "", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
        case ARG_COMPARE:
            compare = true;
            break;

        case ARG_INPUT:
            path = optarg;
            break;

        case ARG_BASE:
            base = atoi(optarg);
            break;

        case ARG_TEST_NAME:
            test_name = optarg;
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
        {
            int i = 0;
            for (auto &&r : src.lists) {
                printf("%d : %s-%s\n", i, r.sysinfo.cpuid.c_str(), r.sysinfo.os.c_str());
                i++;
            }
        }

        for (auto &&b : bench_list) {
            if (b->name == test_name) {
                std::vector<result_ptr_t> results;

                for (auto &&r : src.lists) {
                    auto it = r.results.find(test_name);

                    if (it == r.results.end()) {
                        results.push_back(nullptr);
                    } else {
                        results.push_back(it->second);
                    }
                }

                if (results[base]) {
                    auto rv = b->compare(results, base);
                    for (auto &&r : rv) {
                        printf("== %s ==\n", r.name.c_str());

                        for (auto && d : r.data) {
                            if (r.xlabel_style == XLabelStyle::STRING) {
                                printf("%40s|", d.xlabel.c_str());
                            } else {
                                printf("%40f|", d.xval);
                            }

                            for (auto && dd : d.data) {
                                printf("%8.3f,", (double)dd);
                            }

                            printf("\n");
                        }
                    }
                }
            }
        }
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

                std::cout << "=== " << b->name << " ===\n";
                it->second->dump_human_readable(std::cout,
                                                b->double_precision());
            }
        }
    }

    return 0;
}
