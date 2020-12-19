#pragma once

#include "sys-microbenchmark.h"
#include "picojson.h"

namespace smbm {

namespace json {
inline picojson::value to_jv(int x) { return picojson::value((double)x); }
inline picojson::value to_jv(unsigned int x) {
    return picojson::value((double)x);
}
inline picojson::value to_jv(BenchResult const *r) {
    return r->dump_json();
}

inline picojson::value to_jv(double x) { return picojson::value((double)x); }
inline picojson::value to_jv(std::string const &x) {
    return picojson::value(x);
}

template <typename T> picojson::value to_jv(std::vector<T> const &v) {
    std::vector<picojson::value> ret;
    std::transform(v.begin(), v.end(), std::back_inserter(ret),
                   [](auto x) { return to_jv(x); });
    return picojson::value(ret);
}

inline void from_jv(picojson::value const &jv, int *p) {
    *p = (int)jv.get<double>();
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
template <typename T>
void from_jv(picojson::value const &jv, std::vector<T> *vec) {
    auto jvec = jv.get<picojson::value::array>();
    std::transform(jvec.begin(), jvec.end(), vec->begin(), [](auto x) {
        T y;
        from_jv(x, &y);
        return y;
    });
}

} // namespace json

} // namespace smbm
