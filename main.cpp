#include "sys-microbenchmark.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    using namespace smbm;
    auto bench_list = get_benchmark_list();

    if (argc == 1) {
        for (auto &&b : bench_list) {
            b.run(std::cout);
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
                int i = ai - 1;
                if (bench_list[i].name == argv[ai]) {
                    bench_list[i].run(std::cout);
                }
            }
        }
    }
}
