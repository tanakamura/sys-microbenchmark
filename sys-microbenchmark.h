#pragma once
#include <functional>
#include <string>
#include <vector>
#include <iostream>
#include <memory>

namespace smbm {

constexpr int PRINT_DOUBLE_PRECISION=5;

struct BenchResult {
    virtual void dump_csv(std::ostream &) = 0;
    virtual void dump_human_readable(std::ostream &) = 0;
};

struct BenchDesc {
    std::string name;
    std::function<std::unique_ptr<BenchResult> (std::ostream &)> run;
    std::function<std::unique_ptr<BenchResult> (std::string const &s)> parse_result_csv;
};

#define FOR_EACH_BENCHMARK_LIST(F)\
    F(idiv)\
    F(syscall)\

std::vector<BenchDesc> get_benchmark_list();

#define DEFINE_ENTRY(B) \
    BenchDesc get_##B##_desc();

FOR_EACH_BENCHMARK_LIST(DEFINE_ENTRY)

#define REP8(A) A A A A A A A A
#define REP16(A) REP8(A) REP8(A)


struct perf_counter {
    int fd;

    perf_counter();
    ~perf_counter();

    uint64_t cpu_cycles();
};


} // namespace smbm
