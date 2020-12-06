#include "sys-microbenchmark.h"
#include <iomanip>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    using namespace smbm;
    auto bench_list = get_benchmark_list();

    std::cout << std::fixed << std::setprecision(PRINT_DOUBLE_PRECISION);

    if (argc == 1) {
        for (auto &&b : bench_list) {
            std::cout << "==== " << b.name << " ====\n";
            b.run(std::cout);
            std::cout << "\n\n";
        }

    } else {
        if (strcmp(argv[1], "-h") == 0) {
            puts("usage : sys-microbenchmark [benchmarck-list]");
            puts("  benchmarks:");
            for (auto &&b : bench_list) {
                printf("    %s\n", b.name.c_str());
            }
        } else {
            for (int ai = 1; ai < argc; ai++) {
                for (auto &&b : bench_list) {
                    if (b.name == argv[ai]) {
                        std::cout << "==== " << b.name << " ====\n";
                        b.run(std::cout);
                        std::cout << "\n\n";
                    }
                }
            }
        }
    }
}
