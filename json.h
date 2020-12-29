#pragma once

#include "picojson.h"
#include "sys-microbenchmark.h"

namespace smbm {

namespace json {
inline picojson::value to_jv(int x) { return picojson::value((double)x); }
inline picojson::value to_jv(char x) { return picojson::value((double)x); }
inline picojson::value to_jv(unsigned int x) {
    return picojson::value((double)x);
}
inline picojson::value to_jv(BenchResult const *r) { return r->dump_json(); }
inline picojson::value to_jv(result_ptr_t const &p) { return p->dump_json(); }

inline picojson::value to_jv(double x) { return picojson::value((double)x); }
inline picojson::value to_jv(std::string const &x) {
    return picojson::value(x);
}

picojson::value to_jv(SysInfo const &si);
picojson::value to_jv(ResultList const &rl);

template <typename T> 
picojson::value to_jv(std::map<std::string,picojson::value> const &v) {
    return picojson::value(v);
}


template <typename T> 
picojson::value to_jv(std::map<std::string,T> const &v) {
    std::map<std::string, picojson::value> ret;
    for (auto && e : v) {
        ret.insert(std::make_pair(e.first, to_jv(e.second)));
    }
    return picojson::value(ret);
}

template <typename T> picojson::value to_jv(std::vector<T> const &v) {
    std::vector<picojson::value> ret(v.size());
    std::transform(v.begin(), v.end(), ret.begin(),
                   [](auto x) { return to_jv(x); });
    return picojson::value(ret);
}

inline void from_jv(picojson::value const &jv, int *p) {
    *p = (int)jv.get<double>();
}
inline void from_jv(picojson::value const &jv, char *p) {
    *p = (char)jv.get<double>();
}
inline void from_jv(picojson::value const &jv, unsigned int *p) {
    *p = (unsigned int)jv.get<double>();
}
inline void from_jv(picojson::value const &jv, double *p) {
    *p = (double)jv.get<double>();
}
inline void from_jv(picojson::value const &jv, std::string *p) {
    *p = jv.to_str();
}
inline void from_jv(picojson::value const &jv, bool *p) {
    *p = jv.get<bool>();
}

template <typename T>
void from_jv(picojson::value const &jv, std::vector<T> *vec) {
    auto jvec = jv.get<picojson::value::array>();
    vec->resize(jvec.size());
    std::transform(jvec.begin(), jvec.end(), vec->begin(), [](auto x) {
        T y;
        from_jv(x, &y);
        return y;
    });
}

} // namespace json

} // namespace smbm
