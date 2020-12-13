#pragma once
#include "sys-microbenchmark.h"
#include <iostream>
#include <math.h>
#include <iomanip>
#include <sstream>

namespace smbm {

template <typename T, typename ROW_LABEL_T, typename COLUMN_LABEL_T> struct Table2D;
template <typename T, typename LT> struct Table1D;

template <typename T, typename ROW_LABEL_T, typename COLUMN_LABEL_T>
void dump2d(std::ostream &out, Table2D<T, ROW_LABEL_T, COLUMN_LABEL_T> *t, bool uniform_width,
            int double_precision);

template <typename T, typename LT>
void dump1d(std::ostream &out, Table1D<T, LT> *t, int double_precision);

namespace detail {
inline picojson::value to_jv(int x) { return picojson::value((double)x); }
inline picojson::value to_jv(unsigned int x) {
    return picojson::value((double)x);
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

} // namespace detail

template <typename T, typename ROW_LABEL_T, typename COLUMN_LABEL_T = ROW_LABEL_T> struct Table2D : BenchResult {
    std::string label[2];
    std::vector<T> v;
    std::vector<COLUMN_LABEL_T> column_label;
    std::vector<ROW_LABEL_T> row_label;
    int d1, d0;

    Table2D(std::string const &d1_label, std::string const &d0_label, int d1,
            int d0)
        : label{d0_label, d1_label}, d1(d1), d0(d0) {
        v.resize(d0 * d1);
        column_label.resize(d0);
        row_label.resize(d1);
    }

    T *operator[](int idx) { return &v[idx * d0]; }
    const T *operator[](int idx) const { return &v[idx * d0]; }

    void dump_human_readable(std::ostream &os, int double_precision) override {
        dump2d(os, this, false, double_precision);
    }

    picojson::value dump_json() override {
        typedef picojson::value v_t;
        using namespace detail;

        std::map<std::string, v_t> ret;

        {
            std::vector<v_t> labels = {v_t(label[0]), v_t(label[1])};
            ret["label"] = v_t(labels);
        }

        ret["column_label"] = to_jv(column_label);
        ret["row_label"] = to_jv(row_label);
        ret["values"] = to_jv(v);
        ret["d0"] = to_jv(d0);
        ret["d1"] = to_jv(d1);

        return picojson::value(ret);
    }

    static Table2D<T, ROW_LABEL_T, COLUMN_LABEL_T> *parse_json_result(picojson::value const &value) {
        typedef Table2D<T, ROW_LABEL_T, COLUMN_LABEL_T> ret_t;
        typedef picojson::value v_t;
        using namespace detail;

        v_t const &l = value.get("label");
        std::string label0 = l.get(0).to_str();
        std::string label1 = l.get(1).to_str();

        int d0 = (int)value.get("d0").get<double>();
        int d1 = (int)value.get("d1").get<double>();

        ret_t *r = new Table2D(label1, label0, d1, d0);

        from_jv(l.get("values"), &r->v);

        return r;
    }
};

template <typename T, typename LT> struct Table1D : BenchResult {
    std::string label;
    std::vector<T> v;
    std::vector<LT> row_label;
    std::string column_label;
    int d0;

    Table1D(std::string const &d0_label, int d0) : label(d0_label), d0(d0) {
        v.resize(d0);
        row_label.resize(d0);
    }

    T &operator[](int idx) { return v[idx]; }
    const T &operator[](int idx) const { return v[idx]; }

    picojson::value dump_json() override {
        typedef picojson::value v_t;
        using namespace detail;

        std::map<std::string, v_t> ret;

        ret["column_label"] = v_t(column_label);
        ret["label"] = v_t(label);
        ret["row_label"] = to_jv(row_label);
        ret["values"] = to_jv(v);
        ret["d0"] = to_jv(d0);

        return picojson::value(ret);
    }
    void dump_human_readable(std::ostream &os, int double_precision) override {
        dump1d(os, this, double_precision);
    }

    static Table1D<T, LT> *parse_json_result(picojson::value const &value) {
        using namespace detail;

        std::string label0;
        from_jv(value.get("label"), &label0);

        int d0;
        from_jv(value.get("d0"), &d0);

        auto r = new Table1D(label0, d0);
        from_jv(value.get("values"), &r->v);

        return r;
    }
};

inline unsigned int get_column_width(unsigned int x, int) {
    if (x == 0) {
        return 1;
    }

    return (unsigned int)ceil(log10(x + 1));
}
inline unsigned int get_column_width(int x, int) {
    if (x == 0) {
        return 1;
    }

    return (unsigned int)ceil(log10(x + 1));
}

inline unsigned int get_column_width(double x, int double_precision) {
    std::stringstream ss;

    ss << std::fixed << std::setprecision(double_precision);
    ss << x;

    return ss.str().length();
}

inline unsigned int get_column_width(std::string const &x, int) {
    return x.length();
}

inline void insert_char_n(std::ostream &out, char c, int n) {
    for (int i = 0; i < n; i++) {
        out << c;
    }
}

template <typename T, typename ROW_LABEL_T, typename COLUMN_LABEL_T>
void dump2d(std::ostream &out, Table2D<T, ROW_LABEL_T, COLUMN_LABEL_T> *t, bool uniform_width,
            int double_precision) {
    std::vector<unsigned int> max_column(t->d0, 0);
    unsigned int row_label_max_column = 0;

    std::cout << std::fixed << std::setprecision(double_precision);

    for (int i = 0; i < t->d1; i++) {
        for (int j = 0; j < t->d0; j++) {
            max_column[j] =
                std::max(max_column[j], get_column_width((*t)[i][j], double_precision));
        }
    }

    for (int j = 0; j < t->d0; j++) {
        max_column[j] =
            std::max(max_column[j], get_column_width(t->column_label[j],double_precision));
    }

    for (int i = 0; i < t->d1; i++) {
        row_label_max_column =
            std::max(row_label_max_column, get_column_width(t->row_label[i], double_precision));
    }

    out << "-> : " << t->label[0] << '\n';

    if (uniform_width) {
        unsigned int max = 0;
        for (int j = 0; j < t->d0; j++) {
            max = std::max(max, max_column[j]);
        }
        max = std::max(max, row_label_max_column);

        for (int j = 0; j < t->d0; j++) {
            max_column[j] = max;
        }
        row_label_max_column = max;
    }

    int row_width = 2 + row_label_max_column;

    out << '|';
    insert_char_n(out, ' ', row_label_max_column);
    out << ' ';

    for (int j = 0; j < t->d0; j++) {
        unsigned int nchar = get_column_width(t->column_label[j], double_precision);
        out << '|';
        insert_char_n(out, ' ', max_column[j] - nchar);
        out << t->column_label[j];

        row_width += 1 + max_column[j];
    }

    out << '\n';

    insert_char_n(out, '=', row_width);
    out << '\n';

    for (int i = 0; i < t->d1; i++) {
        {
            unsigned int nchar = get_column_width(t->row_label[i], double_precision);
            out << '|';
            insert_char_n(out, ' ', row_label_max_column - nchar);
            out << t->row_label[i];
        }

        out << ' ';

        for (int j = 0; j < t->d0; j++) {
            T v = (*t)[i][j];
            unsigned int nchar = get_column_width(v, double_precision);
            out << '|';
            insert_char_n(out, ' ', max_column[j] - nchar);
            out << v;
        }

        out << '\n';

        insert_char_n(out, '-', row_width);
        out << '\n';
    }

    out << "v : " << t->label[1] << '\n';
}

template <typename T, typename LT>
void dump1d(std::ostream &out, Table1D<T, LT> *t, int double_precision) {
    unsigned int max_column = 0;
    unsigned int row_label_max_column = 0;

    for (int i = 0; i < t->d0; i++) {
        max_column = std::max(max_column, get_column_width((*t)[i], double_precision));
    }

    max_column = std::max(max_column, get_column_width(t->column_label, double_precision));

    for (int i = 0; i < t->d0; i++) {
        row_label_max_column =
            std::max(row_label_max_column, get_column_width(t->row_label[i], double_precision));
    }

    int row_width = 3 + row_label_max_column + max_column;

    out << '|';
    insert_char_n(out, ' ', row_label_max_column);
    out << ' ';
    out << '|';
    out << t->column_label;

    out << '\n';

    insert_char_n(out, '=', row_width);
    out << '\n';

    for (int i = 0; i < t->d0; i++) {
        {
            unsigned int nchar = get_column_width(t->row_label[i], double_precision);
            out << '|';
            insert_char_n(out, ' ', row_label_max_column - nchar);
            out << t->row_label[i];
        }

        out << ' ';

        T v = (*t)[i];
        unsigned int nchar = get_column_width(v, double_precision);
        out << '|';
        insert_char_n(out, ' ', max_column - nchar);
        out << v;

        out << '\n';

        insert_char_n(out, '-', row_width);
        out << '\n';
    }

    out << "v : " << t->label << '\n';
}

} // namespace smbm
