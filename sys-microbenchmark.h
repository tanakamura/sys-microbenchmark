#pragma once
#include <functional>
#include <string>
#include <vector>
#include <iostream>
#include <memory>

#include "picojson.h"

namespace smbm {

constexpr int PRINT_DOUBLE_PRECISION=5;

struct BenchResult {
    virtual picojson::value dump_json() = 0;
    virtual void dump_human_readable(std::ostream &) = 0;
};

struct BenchDesc {
    std::string name;
    typedef std::unique_ptr<BenchResult> result_t;
    BenchDesc(std::string const &s)
        :name(s)
    {}
    virtual ~BenchDesc() { }
    virtual result_t run() = 0;
    virtual result_t parse_json_result(picojson::value const &v) = 0;
};

#define FOR_EACH_BENCHMARK_LIST(F)\
    F(idiv32)\
    F(idiv64)\
    F(syscall)\

std::vector< std::unique_ptr<BenchDesc> > get_benchmark_list();

#define DEFINE_ENTRY(B) \
    std::unique_ptr<BenchDesc> get_##B##_desc();

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
