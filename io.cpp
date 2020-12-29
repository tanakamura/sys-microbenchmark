#include "sys-microbenchmark.h"
#include "json.h"

namespace smbm {

using namespace picojson;
using namespace json;

void merge_result(ResultListSet &dst,
                  result_set_t const &results,
                  SysInfo const &insobj_sysinfo)
{
    using namespace json;

    result_set_t merge;

    for (auto it = dst.lists.begin(); it!=dst.lists.end(); ) {
        SysInfo &s0 = it->sysinfo;

        bool match = true;

        match &= (s0.cpuid == insobj_sysinfo.cpuid);
        match &= (s0.os == insobj_sysinfo.os);

        if (match) {
            for (auto &&r : it->results) {
                merge.insert(std::make_pair(r.first,r.second));
            }

            it = dst.lists.erase(it);
        } else {
            ++it;
        }
    }


    for (auto &&r : results) {
        merge.insert(std::make_pair(r.first,r.second));
    }

    auto &e = dst.lists.emplace_back(ResultList());

    e.sysinfo = insobj_sysinfo;
    e.results = merge;
}



picojson::value serialize_result(ResultListSet const &src)
{
    picojson::value v = to_jv(src.lists);


    return picojson::value(v);
}


void
deserialize_result(ResultListSet &dst, picojson::value const &r)
{
    auto setobj = r.get<array>();

    auto bench_list = get_all_benchmark_list();

    for (auto list : setobj) {
        auto listobj = list.get<object>();

        auto sysinfo = listobj["sysinfo"].get<object>();
        auto results = listobj["results"].get<object>();

        ResultList &rl = dst.lists.emplace_back(ResultList());
        SysInfo &si = rl.sysinfo;
        result_set_t &rs = rl.results;

        from_jv(sysinfo["cpuid"], &si.cpuid);
        from_jv(sysinfo["os"], &si.os);
        from_jv(sysinfo["date"], &si.date);
        from_jv(sysinfo["ostimer"], &si.ostimer);
        from_jv(sysinfo["userland_timer"], &si.userland_timer);
        from_jv(sysinfo["perf_counter_available"], &si.perf_counter_available);
        from_jv(sysinfo["ooo_ratio"], &si.ooo_ratio);
        from_jv(sysinfo["vulnerabilities"], &si.vulnerabilities);

        for (auto && b : bench_list) {
            auto &name = b->name;
            auto it = results.find(name);
            if (it != results.end()) {
                auto &json_result = it->second;
                rs[name] = b->parse_json_result(json_result);
            }
        }
    }
}

namespace json {

picojson::value to_jv(SysInfo const &si) {
    std::map<std::string, picojson::value> json_sysinfo_map;
    json_sysinfo_map["cpuid"] = to_jv(si.cpuid);
    json_sysinfo_map["os"] = to_jv(si.os);
    json_sysinfo_map["date"] = to_jv(si.date);
    json_sysinfo_map["ostimer"] = to_jv(si.ostimer);
    json_sysinfo_map["userland_timer"] = to_jv(si.userland_timer);
    json_sysinfo_map["perf_counter_available"] = to_jv(si.perf_counter_available);
    json_sysinfo_map["ooo_ratio"] = to_jv(si.ooo_ratio);
    json_sysinfo_map["vulnerabilities"] = to_jv(si.vulnerabilities);

    return picojson::value(json_sysinfo_map);
}

picojson::value to_jv(ResultList const &rl) {
    std::map<std::string, picojson::value> m;
    m["sysinfo"] = to_jv(rl.sysinfo);
    m["results"] = to_jv(rl.results);

    return picojson::value(m);
}


}

}
