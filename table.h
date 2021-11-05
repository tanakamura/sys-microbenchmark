#pragma once
#include "json.h"
#include "sys-microbenchmark.h"
#include <iomanip>
#include <iostream>
#include <math.h>
#include <optional>
#include <sstream>

namespace smbm {

template <typename T, typename ROW_LABEL_T, typename COLUMN_LABEL_T>
struct Table2D;
template <typename T, typename LT> struct Table1D;

template <typename T, typename ROW_LABEL_T, typename COLUMN_LABEL_T>
void dump2d(std::ostream &out, Table2D<T, ROW_LABEL_T, COLUMN_LABEL_T> *t,
            bool uniform_width, int double_precision);

template <typename T, typename LT>
void dump1d(std::ostream &out, Table1D<T, LT> *t, int double_precision);

template <typename T> void set_xlabel_style(ComparableResult &cr);
template <> inline void set_xlabel_style<std::string>(ComparableResult &cr) {
    cr.xlabel_style = XLabelStyle::STRING;
}
template <> inline void set_xlabel_style<double>(ComparableResult &cr) {
    cr.xlabel_style = XLabelStyle::DOUBLE;
}
template <> inline void set_xlabel_style<int>(ComparableResult &cr) {
    cr.xlabel_style = XLabelStyle::DOUBLE;
}

inline void set_xlabel(CompareData &cd, const std::string &val) {
    cd.xlabel = val;
}
inline void set_xlabel(CompareData &cd, double val) { cd.xval = val; }

template <typename T, typename ROW_LABEL_T,
          typename COLUMN_LABEL_T = ROW_LABEL_T>
struct Table2D : public BenchResult {
    typedef Table2D<T, ROW_LABEL_T, COLUMN_LABEL_T> This_t;

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

    picojson::value dump_json() const override {
        typedef picojson::value v_t;
        using namespace json;

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

    static Table2D<T, ROW_LABEL_T, COLUMN_LABEL_T> *
    parse_json_result(picojson::value const &value) {
        typedef Table2D<T, ROW_LABEL_T, COLUMN_LABEL_T> ret_t;
        typedef picojson::value v_t;
        using namespace json;

        v_t const &l = value.get("label");
        std::string label0 = l.get(0).to_str();
        std::string label1 = l.get(1).to_str();

        int d0 = (int)value.get("d0").get<double>();
        int d1 = (int)value.get("d1").get<double>();

        ret_t *r = new Table2D(label1, label0, d1, d0);

        from_jv(value.get("values"), &r->v);
        from_jv(value.get("column_label"), &r->column_label);
        from_jv(value.get("row_label"), &r->row_label);

        return r;
    }

    static std::optional<double> find_min_max(result_ptr_t const &r, bool min) {
        if (!r) {
            return std::nullopt;
        }

        auto result = std::dynamic_pointer_cast<This_t>(r);
        if (!result) {
            return std::nullopt;
        }

        size_t nr = result->d1;
        size_t nc = result->d0;

        double val = 0;
        if (min) {
            val = 1e9;
        } else {
            val = -1e9;
        }

        for (size_t ri=0; ri<nr; ri++) {
            for (size_t ci=0; ci<nc; ci++) {
                double vv = result->v[ri*nc + ci];

                if (min) {
                    val = std::min(val, vv);
                } else {
                    val = std::max(val, vv);
                }
            }
        }

        return val;
    }


    static std::vector<ComparableResult>
    compare_minmax(std::string const &name, std::vector<result_ptr_t> const &results, int base_index) {
        auto base_result =
            std::dynamic_pointer_cast<This_t>(results[base_index]);
        size_t nv = base_result->v.size();
        size_t nr = results.size();
        std::vector<ComparableResult> ret(nv);
        ComparableResult cr;

        cr.xlabel_style = XLabelStyle::STRING;
        cr.name = name;
        cr.value_unit = "";

        for (int mm=0; mm<2; mm++) {
            bool min = mm==0;
            CompareData cd;

            auto baseval_o = This_t::find_min_max(base_result, min);
            if (!baseval_o) {
                continue;
            }
            auto baseval = *baseval_o;

            if (min) {
                cd.xlabel = "min";
            } else {
                cd.xlabel = "max";
            }

            for (size_t ri = 0; ri < nr; ri++) {
                auto v = This_t::find_min_max(results[ri], min);
                if (v) {
                    cd.data.push_back((*v) / baseval);
                } else {
                    cd.data.push_back(-1);
                }
            }

            cr.data.push_back(cd);
        }

        return {cr};
    }
};

template <typename T, typename LT> struct Table1D : public BenchResult {
    std::string label;
    std::vector<T> v;
    std::vector<LT> row_label;
    std::string column_label;
    int d0;
    typedef Table1D<T, LT> This_t;

    Table1D(std::string const &d0_label, int d0) : label(d0_label), d0(d0) {
        v.resize(d0);
        row_label.resize(d0);
    }

    T &operator[](int idx) { return v[idx]; }
    const T &operator[](int idx) const { return v[idx]; }

    picojson::value dump_json() const override {
        typedef picojson::value v_t;
        using namespace json;

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
        using namespace json;

        std::string label0;
        from_jv(value.get("label"), &label0);

        int d0;
        from_jv(value.get("d0"), &d0);

        auto r = new Table1D(label0, d0);
        from_jv(value.get("values"), &r->v);
        from_jv(value.get("row_label"), &r->row_label);

        return r;
    }

    static std::optional<double> find_result(result_ptr_t const &r, const LT &label) {
        if (!r) {
            return std::nullopt;
        }

        auto result = std::dynamic_pointer_cast<This_t>(r);
        if (!result) {
            return std::nullopt;
        }

        size_t nl = result->row_label.size();
        for (size_t li = 0; li < nl; li++) {
            if (result->row_label[li] == label) {
                return result->v[li];
            }
        }

        return std::nullopt;
    }

    static std::vector<ComparableResult>
    compare(std::string const &name, std::vector<result_ptr_t> const &results, int base_index) {
        auto base_result =
            std::dynamic_pointer_cast<This_t>(results[base_index]);
        size_t nv = base_result->v.size();
        size_t nr = results.size();
        std::vector<ComparableResult> ret(nv);
        ComparableResult cr;

        set_xlabel_style<LT>(cr);
        cr.name = name;
        cr.value_unit = base_result->column_label;

        for (size_t vi = 0; vi < nv; vi++) {
            CompareData cd;
            double baseval = base_result->v[vi];

            set_xlabel(cd, base_result->row_label[vi]);

            for (size_t ri = 0; ri < nr; ri++) {
                auto v = This_t::find_result(results[ri],
                                             base_result->row_label[vi]);
                if (v) {
                    cd.data.push_back((*v) / baseval);
                } else {
                    cd.data.push_back(-1);
                }
            }

            cr.data.push_back(cd);
        }

        return {cr};
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
inline unsigned int get_column_width(char x, int) { return 1; }

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
void dump2d(std::ostream &out, Table2D<T, ROW_LABEL_T, COLUMN_LABEL_T> *t,
            bool uniform_width, int double_precision) {
    std::vector<unsigned int> max_column(t->d0, 0);
    unsigned int row_label_max_column = 0;

    std::cout << std::fixed << std::setprecision(double_precision);

    for (int i = 0; i < t->d1; i++) {
        for (int j = 0; j < t->d0; j++) {
            max_column[j] = std::max(
                max_column[j], get_column_width((*t)[i][j], double_precision));
        }
    }

    for (int j = 0; j < t->d0; j++) {
        max_column[j] =
            std::max(max_column[j],
                     get_column_width(t->column_label[j], double_precision));
    }

    for (int i = 0; i < t->d1; i++) {
        row_label_max_column =
            std::max(row_label_max_column,
                     get_column_width(t->row_label[i], double_precision));
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
        unsigned int nchar =
            get_column_width(t->column_label[j], double_precision);
        out << '|';
        insert_char_n(out, ' ', max_column[j] - nchar);
        out << t->column_label[j];

        row_width += 1 + max_column[j];
    }

    out << '\n';

    insert_char_n(out, '-', row_width);
    out << '\n';

    for (int i = 0; i < t->d1; i++) {
        {
            unsigned int nchar =
                get_column_width(t->row_label[i], double_precision);
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

        // insert_char_n(out, '-', row_width);
        // out << '\n';
    }

    out << "v : " << t->label[1] << '\n';
}

template <typename T, typename LT>
void dump1d(std::ostream &out, Table1D<T, LT> *t, int double_precision) {
    unsigned int max_column = 0;
    unsigned int row_label_max_column = 0;

    for (int i = 0; i < t->d0; i++) {
        max_column =
            std::max(max_column, get_column_width((*t)[i], double_precision));
    }

    max_column = std::max(max_column,
                          get_column_width(t->column_label, double_precision));

    for (int i = 0; i < t->d0; i++) {
        row_label_max_column =
            std::max(row_label_max_column,
                     get_column_width(t->row_label[i], double_precision));
    }

    int row_width = 3 + row_label_max_column + max_column;

    out << '|';
    insert_char_n(out, ' ', row_label_max_column);
    out << ' ';
    out << '|';
    out << t->column_label;

    out << '\n';

    insert_char_n(out, '-', row_width);
    out << '\n';

    for (int i = 0; i < t->d0; i++) {
        {
            unsigned int nchar =
                get_column_width(t->row_label[i], double_precision);
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

        // insert_char_n(out, '-', row_width);
        // out << '\n';
    }

    out << "v : " << t->label << '\n';
}

template <typename T, typename LT> struct Table1DBenchDesc : public BenchDesc {
    bool low_better;
    Table1DBenchDesc(std::string const &name, bool lower_is_better) : BenchDesc(name), low_better(lower_is_better) {}
    typedef Table1D<T, LT> table_t;

    result_t parse_json_result(picojson::value const &v) override {
        return result_t(table_t::parse_json_result(v));
    }
    std::vector<ComparableResult>
    compare(std::vector<result_ptr_t> const &results, int base_index) override {
        return table_t::compare(this->name, results, base_index);
    }

    bool lower_is_better() const override {
        return low_better;
    }
};

template <typename T, typename RLT, typename CLT = RLT>
struct Table2DBenchDesc : public BenchDesc {
    bool low_better;
    Table2DBenchDesc(std::string const &name, bool lower_is_better) : BenchDesc(name), low_better(lower_is_better) {}
    typedef Table2D<T, RLT, CLT> table_t;

    result_t parse_json_result(picojson::value const &v) override {
        return result_t(table_t::parse_json_result(v));
    }
    std::vector<ComparableResult>
    compare(std::vector<result_ptr_t> const &results, int base_index) override {
        return table_t::compare_minmax(this->name, results, base_index);
    }
    bool lower_is_better() const override {
        return low_better;
    }
};

} // namespace smbm
