#ifdef JS_INTERFACE
#include <emscripten/bind.h>

#include "sys-microbenchmark.h"

using namespace smbm;

using namespace emscripten;

namespace {
std::shared_ptr<SysInfo> get_sysinfo_ptr(std::shared_ptr<GlobalState> &g) {
    auto ret = std::make_shared<SysInfo>();
    *ret = get_sysinfo(g.get());
    return ret;
}
} // namespace

EMSCRIPTEN_BINDINGS(sys_microbenchmark) {
    class_<GlobalState>("GlobalState")
        .smart_ptr_constructor("GlobalState", &std::make_shared<GlobalState>)
        .function("get_active_benchmark_list",
                  &GlobalState::get_active_benchmark_list);

    class_<ResultListSet>("ResultListSet");
    class_<SysInfo>("SysInfo")
        .smart_ptr_constructor("SysInfo", &get_sysinfo_ptr)
        .property("ostimer", &SysInfo::get_ostimer)
        .property("userland_timer", &SysInfo::get_userland_timer)
        .property("os", &SysInfo::get_os)
        .property("date", &SysInfo::get_date)
        .property("vulnerabilities", &SysInfo::get_vulnerabilities)
        .property("cpuid", &SysInfo::get_cpuid)
        .property("perf_counter_available",
                  &SysInfo::get_perf_counter_available)
        .property("ooo_ratio", &SysInfo::get_ooo_ratio);

    class_<BenchDesc>("BenchDesc")
        .smart_ptr<std::shared_ptr<BenchDesc>>("BenchDesc")
        .property("name", &BenchDesc::get_name);

    register_vector<std::shared_ptr<BenchDesc>>("bench_list");
    register_vector<std::string>("vector<string>");

    function("get_all_benchmark_list", &get_all_benchmark_list);
}
#endif
