#include "sys-microbenchmark.h"
#include <iomanip>
#include <stdio.h>
#include <string.h>

#ifdef X86
#include <cpuid.h>
#endif

#include <getopt.h>

static void
run(smbm::GlobalState *g,
    std::map<std::string, picojson::value> *this_obj,
    std::shared_ptr<smbm::BenchDesc> const &b)
{
    std::cout << "==== " << b->name << " ====\n";
    {
        auto result = b->run(g);
        std::cout << std::fixed << std::setprecision(b->double_precision());
        result->dump_human_readable(std::cout, b->double_precision());
        (*this_obj)[b->name] = result->dump_json();
    }
    std::cout << "\n\n";
}

int main(int argc, char **argv) {
    using namespace smbm;

    GlobalState g;
    auto bench_list = g.get_benchmark_list();

    std::string json_path = "result.json";

    {
        while (1) {
            static struct option long_options[] = {
                {"help", no_argument, 0, 'h'},
                {"result", required_argument, 0, 'R'},

                {0,0,0,0},
            };

            int option_index = 0;

            int c = getopt_long(argc, argv, "hR:",
                                long_options, &option_index);

            if (c == -1) {
                break;
            }

            switch (c) {
            case 'h':
            default:
                printf("usage : %s [-R result.json] [test-list]", argv[0]);
                fflush(stdout);
                puts("  tests:");
                for (auto &&b : bench_list) {
                    printf("    %s\n", b->name.c_str());
                }
                return 0;

            case 'R':
                json_path = optarg;
                break;
            }
        }

    }

    std::map<std::string, picojson::value> result_obj;
    picojson::value sys_info = get_sysinfo(&g);

    warmup_thread(&g);

    if (optind == argc) {
        for (auto &&b : bench_list) {
            run(&g, &result_obj, b);
        }
    } else {
        for (int ai = 1; ai < argc; ai++) {
            for (auto &&b : bench_list) {
                if (b->name == argv[ai]) {
                    run(&g, &result_obj, b);
                }
            }
        }
    }

    {
        picojson::value root = load_result(json_path);
        root = insert_result(root, picojson::value(result_obj), sys_info);
        save_result(json_path, root);
    }
}
