#ifdef JS_INTERFACE
#include <emscripten/bind.h>
#include <sstream>
#include <iomanip>

#include "sys-microbenchmark.h"

using namespace smbm;

using namespace emscripten;

namespace {
std::shared_ptr<SysInfo> get_sysinfo_ptr(std::shared_ptr<GlobalState> &g) {
    auto ret = std::make_shared<SysInfo>();
    *ret = get_sysinfo(g.get());
    return ret;
}

std::shared_ptr<ResultListSet> new_result_list_set() {
    return std::make_shared<ResultListSet>();
}

void deserialize_result_sp(std::shared_ptr<ResultListSet> &p,
                           std::string const &json) {
    std::istringstream iss(json);
    picojson::value v;
    iss >> v;
    deserialize_result(*p, v);
}

std::string get_human_readable(BenchResult *r, double prec)
{
  std::stringstream ss;
  ss << std::fixed << std::setprecision(prec);
  r->dump_human_readable(ss, prec);
  return ss.str();
}

} // namespace

EMSCRIPTEN_BINDINGS(sys_microbenchmark) {
    class_<GlobalState>("GlobalState")
        .smart_ptr_constructor("GlobalState", &std::make_shared<GlobalState>)
        .function("get_active_benchmark_list",
                  &GlobalState::get_active_benchmark_list, allow_raw_pointer<ret_val>());

    class_<ResultListSet>("ResultListSet")
        .smart_ptr_constructor("ResultListSet", &new_result_list_set)
        .function("get_lists", &ResultListSet::get_lists, allow_raw_pointer<ret_val>());

    class_<ResultList>("ResultList")
        .function("get_results", &ResultList::get_results, allow_raw_pointer<ret_val>())
        .function("get_sysinfo", &ResultList::get_sysinfo, allow_raw_pointer<ret_val>());

    class_<BenchResult>("BenchResult")
        .smart_ptr<std::shared_ptr<BenchResult>>("BenchResult")
        .function("get_human_readable", &get_human_readable, allow_raw_pointer<arg<0>>());

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
        .property("name", &BenchDesc::get_name)
        .function("compare", &BenchDesc::compare)
        .function("get_double_precision", &BenchDesc::double_precision);

    class_<ComparableResult>("ComparableResult")
        .function("get_data", &ComparableResult::get_data,
                  allow_raw_pointer<ret_val>())
        .function("get_value_unit", &ComparableResult::get_value_unit);

    class_<CompareData>("CompareData")
        .function("get_double_xlabel", &CompareData::get_double_xlabel)
        .function("get_str_xlabel", &CompareData::get_str_xlabel)
        .function("get_data", &CompareData::get_data, allow_raw_pointer<ret_val>());

    register_vector<std::shared_ptr<BenchDesc>>("bench_list");
    register_vector<double>("vector_double");
    register_vector<std::string>("vector_string");
    register_vector<ResultList>("vector_ResultList");
    register_vector<result_ptr_t>("vector_Result");
    register_vector<ComparableResult>("vector_ComparableResult");
    register_vector<CompareData>("vector_CompareData");
    register_map<std::string, result_ptr_t>("map_string_result_ptr_t");

    function("get_all_benchmark_list", &get_all_benchmark_list);
    function("merge_result", &merge_result);
    function("deserialize_result", &deserialize_result_sp, allow_raw_pointer<arg<0> >());
}
#endif
