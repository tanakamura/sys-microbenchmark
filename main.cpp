#include "sys-microbenchmark.h"
#include <iomanip>
#include <stdio.h>
#include <string.h>

#ifdef X86
#include <cpuid.h>
#endif

#include <fstream>
#include <getopt.h>

#ifdef POSIX
#include <sys/utsname.h>
#endif

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

#ifdef X86
#define x_cpuid(p,eax) __get_cpuid(eax, &(p)[0], &(p)[1], &(p)[2], &(p)[3]);
    typedef unsigned int cpuid_t;
    cpuid_t data[4*3+1];
#endif


    std::string json_path = "result.json";

    picojson::value root;

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

        std::ifstream ifs;
        ifs.open(json_path);
        if (ifs) {
            ifs >> root;
        } else {
            root = picojson::value( (picojson::value::array){} );
        }
    }

    std::map<std::string, picojson::value> this_obj;

    this_obj["ostimer"] = picojson::value(ostimer_value::name());
    this_obj["userland_timer"] = picojson::value(userland_timer_value::name());
    this_obj["perf_counter_available"] = picojson::value(g.is_hw_perf_counter_available());

    std::cout << "ostimer: " << ostimer_value::name() << '\n';
    std::cout << "userland_timer: " << userland_timer_value::name() << '\n';
    std::cout << "perf_counter: " << (g.is_hw_perf_counter_available()?"yes":"no") << '\n';

#ifdef X86
    x_cpuid(data+4*0, 0x80000002);
    x_cpuid(data+4*1, 0x80000003);
    x_cpuid(data+4*2, 0x80000004);
    data[12] = 0;

    this_obj["cpuid"] = picojson::value((char*)data);
    puts((char*)data);
#else
    this_obj["cpuid"] = picojson::value("unknown");
#endif

    warmup_thread(&g);

    {
        char buffer[256];
        time_t now = time(NULL);
        static struct tm *t = localtime(&now);
        strftime(buffer, 1024, "%Y-%m-%dT%H:%M:%d%z", t);
        this_obj["run_time"] = picojson::value(buffer);
    }

#ifdef __linux__
    {
        std::ifstream cmdline;
        cmdline.open("/proc/cmdline");
        std::string buffer;

        std::getline(cmdline, buffer);
        this_obj["kernel_cmdline"] = picojson::value(buffer);
    }
#else
    this_obj["kernel_cmdline"] = picojson::value("");
#endif

#ifdef POSIX
    {
        struct utsname n;
        uname(&n);
        std::string uname = std::string(n.sysname) + n.release + n.version + n.machine;
        this_obj["os"] = picojson::value(uname);
    }

#elif defined WINDOWS
    this_obj["os"] = picojson::value("Windows");

#endif

    if (optind == argc) {
        for (auto &&b : bench_list) {
            run(&g, &this_obj, b);
        }
    } else {
        for (int ai = 1; ai < argc; ai++) {
            for (auto &&b : bench_list) {
                if (b->name == argv[ai]) {
                    run(&g, &this_obj, b);
                }
            }
        }
    }

    {
        auto vec = root.get<picojson::value::array>();

        for (auto it = vec.begin(); it != vec.end();) {
            std::string cpuid0 = it->get("cpuid").get<std::string>();
            std::string cmdline0 = it->get("kernel_cmdline").get<std::string>();

            std::string cpuid1 = this_obj["cpuid"].get<std::string>();
            std::string cmdline1 = this_obj["kernel_cmdline"].get<std::string>();

            if (cpuid0 == cpuid1 && cmdline0==cmdline1) {
                it = vec.erase(it);
            } else {
                ++it;
            }
        }

        vec.push_back( picojson::value(this_obj) );

        picojson::value rootj(vec);

        std::ofstream ofs;
        ofs.open("result.json");
        ofs << rootj;
    }
}
