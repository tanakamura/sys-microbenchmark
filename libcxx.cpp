#include "features.h"
#include <chrono>
#include <map>
#include <random>
#include <sstream>
#include <variant>

#ifdef HAVE_THREAD
#include <mutex>
#include <thread>
#endif

#include "barrier.h"
#include "oneshot_timer.h"
#include "simple-run.h"
#include "sys-microbenchmark.h"
#include "table.h"

namespace smbm {

namespace {

struct no_arg {
    void *alloc_arg() { return nullptr; };
    void free_arg(void *p) {}
};

struct alloc_ostringstream : public no_arg {
    void run(GlobalState const *g, void *p) { std::ostringstream oss; }
};

struct ostringstream_int_99999 {
    std::ostringstream *alloc_arg() { return new std::ostringstream(); }
    void free_arg(std::ostringstream *os) { delete os; }

    void run(GlobalState const *g, std::ostringstream *os) { (*os) << 99999; }
};

struct ostringstream_double_99999 {
    std::ostringstream *alloc_arg() { return new std::ostringstream(); }
    void free_arg(std::ostringstream *os) { delete os; }

    void run(GlobalState const *g, std::ostringstream *os) { (*os) << 99999.0; }
};
struct ostringstream_double_033333 {
    std::ostringstream *alloc_arg() { return new std::ostringstream(); }
    void free_arg(std::ostringstream *os) { delete os; }

    void run(GlobalState const *g, std::ostringstream *os) {
        (*os) << (1.0 / 3.0);
    }
};

struct alloc_istringstream : public no_arg {
    void run(GlobalState const *g, void *p) { std::istringstream iss(""); }
};

struct istringstream_int_99999 {
    std::istringstream *alloc_arg() { return new std::istringstream("99999"); }
    void free_arg(std::istringstream *os) { delete os; }

    void run(GlobalState const *g, std::istringstream *is) {
        is->seekg(0, is->beg);
        int v;
        (*is) >> v;
        if (v != 99999) {
            abort();
        }
    }
};
struct istringstream_double_99999 {
    std::istringstream *alloc_arg() { return new std::istringstream("99999"); }
    void free_arg(std::istringstream *os) { delete os; }

    void run(GlobalState const *g, std::istringstream *is) {
        is->seekg(0, is->beg);
        double v;
        (*is) >> v;
        if (v != 99999.0) {
            abort();
        }
    }
};

struct iomanip_set_precision {
    std::ostringstream *alloc_arg() { return new std::ostringstream(); }
    void free_arg(std::ostringstream *os) { delete os; }

    void run(GlobalState const *g, std::ostringstream *os) {
        (*os) << std::fixed << std::setprecision(4);
    }
};

struct alloc_string : public no_arg {
    void run(GlobalState const *g, void *arg) {
        std::string h = "Hello World";
        g->dummy_write(0, h[0]);
    }
};

struct concat_hello_world : public no_arg {
    void run(GlobalState const *g, void *arg) {
        std::string h = "Hello";
        std::string w = "World";

        std::string hw = h + " " + w;

        g->dummy_write(0, hw[0]);
    }
};

struct new_delete_1byte : public no_arg {
    void run(GlobalState const *g, void *arg) {
        char *p = new char;
        delete p;
    }
};

struct unique_ptr_reset {
    void *alloc_arg() {
        int *p = new int;
        return p;
    };
    void free_arg(void *p) { delete (int *)p; }

    void run(GlobalState const *g, void *arg) {
        int *p = (int *)arg;
        std::unique_ptr<int> up;

        up.reset(p);
        up.release();
    }
};

struct shared_ptr_incdec {
    void *alloc_arg() {
        int *p = new int;
        std::shared_ptr<int> *sp = new std::shared_ptr<int>(p);
        return sp;
    };
    void free_arg(void *p) { delete (std::shared_ptr<int> *)p; }

    void run(GlobalState const *g, void *arg) {
        std::shared_ptr<int> x = *(std::shared_ptr<int> *)arg;
        g->dummy_write(0, x.use_count());
    }
};

struct map_insert_remove {
    int size;
    int insval;
    map_insert_remove(int size, int insval) : size(size), insval(insval) {}

    void *alloc_arg() {
        std::map<int, int> *p = new std::map<int, int>;

        for (int i = 0; i < size; i++) {
            p->insert(std::make_pair(i, i));
        }

        return p;
    }

    void free_arg(void *p) { delete (std::map<int, int> *)p; }

    void run(GlobalState const *g, void *arg) {
        auto m = (std::map<int, int> *)arg;
        int v = this->insval;

        (*m).insert(std::make_pair(v, v));

        auto it = m->find(v);
        if (it != m->end()) {
            m->erase(it);
        }
    }
};

struct map1M_insert_remove : public map_insert_remove {
    map1M_insert_remove() : map_insert_remove(1024 * 1024, 555555) {}
};

struct map4_insert_remove : public map_insert_remove {
    map4_insert_remove() : map_insert_remove(4, 2) {}
};

struct SortData {
    int *orig;
    int *cur;
};

template <bool stable> struct sort {
    int size;

    sort(int size) : size(size) {}

    void *alloc_arg() {
        int *p = new int[size];
        for (int i = 0; i < size; i++) {
            p[i] = drand48() * 1024 * 1024 * 16;
        }

        struct SortData *ret = new SortData;
        ret->orig = p;
        ret->cur = new int[size];

        return ret;
    }

    void free_arg(void *p) {
        struct SortData *d = (struct SortData *)p;

        delete[] d->orig;
        delete[] d->cur;

        delete d;
    }

    void run(GlobalState const *g, void *arg) {
        struct SortData *d = (struct SortData *)arg;
        std::copy(d->orig, d->orig + size, d->cur);

        if (stable) {
            std::stable_sort(d->cur, d->cur + size);
        } else {
            std::sort(d->cur, d->cur + size);
        }
    }
};

struct sort1K : public sort<false> {
    sort1K() : sort(1024) {}
};
struct sort4 : public sort<false> {
    sort4() : sort(4) {}
};

struct stable_sort1K : public sort<true> {
    stable_sort1K() : sort(1024) {}
};
struct stable_sort4 : public sort<true> {
    stable_sort4() : sort(4) {}
};

struct steady_clock_now : public no_arg {
    void run(GlobalState const *g, void *arg) {
        std::chrono::steady_clock::now();
    }
};
struct system_clock_now : public no_arg {
    void run(GlobalState const *g, void *arg) {
        std::chrono::system_clock::now();
    }
};

template <typename T> struct rand_base {
    T *alloc_arg() { return new T(0); }
    void free_arg(T *p) { delete p; }

    void run(GlobalState const *g, T *r) { g->dummy_write(0, (*r)()); }
};

struct minstd_rand : public rand_base<std::minstd_rand> {};
struct mt19937_rand : public rand_base<std::mt19937> {};
struct default_rand : public rand_base<std::default_random_engine> {};

struct steady_timepoint_to_sec : public no_arg {
    void *alloc_arg() {
        using namespace std::chrono;

        steady_clock::time_point *tlist = new steady_clock::time_point[2];

        tlist[0] = steady_clock::now();
        tlist[1] = steady_clock::now();

        return tlist;
    }

    void free_arg(void *p) {
        using namespace std::chrono;
        delete[](steady_clock::time_point *) p;
    }

    void run(GlobalState const *g, void *arg) {
        using namespace std::chrono;
        auto tlist = (steady_clock::time_point *)arg;
        auto d = tlist[1] - tlist[0];
        auto sec = duration_cast<seconds>(d);
        g->dummy_write(0, sec.count());
    }
};

struct variant_get {
    typedef std::variant<char, short, int, long, float, double> v_t;

    v_t *alloc_arg() {
        v_t *ret = new v_t(1);
        return ret;
    }

    void free_arg(v_t *v) { delete v; }

    void run(GlobalState const *g, v_t *arg) {
        g->dummy_write(0, std::get<int>(*arg));
    }
};

struct typeid_get {
    struct A {
        virtual ~A() {}
    };

    A *alloc_arg() { return new A; }

    void free_arg(A *v) { delete v; }

    void run(GlobalState const *g, A *arg) {
        auto &ti = typeid(*arg);
        g->dummy_write(0, ti.hash_code());
    }
};

#ifdef HAVE_THREAD
struct scoped_lock_unlock : public no_arg {
    void *alloc_arg() {
        std::mutex *p = new std::mutex();
        return (void *)p;
    };
    void free_arg(void *p) {
        std::mutex *m = (std::mutex *)p;
        delete m;
    }

    void run(GlobalState const *g, void *arg) {
        std::mutex *m = (std::mutex *)arg;
        std::lock_guard<std::mutex> l(*m);
    }
};
#endif

#ifdef __cpp_exceptions

struct enter_try : public no_arg {
    void run(GlobalState const *g, void *arg) {
        try {
            compiler_mb();
        } catch (...) {
        }
    }
};

struct throw_catch : public no_arg {
    void run(GlobalState const *g, void *arg) {
        try {
            throw "x";
        } catch (...) {
            compiler_mb();
        }
    }
};

#endif

#ifdef HAVE_THREAD

#define FOR_EACH_TEST_THREAD(F) F(scoped_lock_unlock)

#else

#define FOR_EACH_TEST_THREAD(F)

#endif

#ifdef __cpp_exceptions

#define FOR_EACH_TEST_EXCEPTIONS(F)                                            \
    F(enter_try)                                                               \
    F(throw_catch)

#else

#define FOR_EACH_TEST_EXCEPTIONS(F)

#endif

#define FOR_EACH_TEST_GENERIC(F)                                               \
    F(alloc_ostringstream)                                                     \
    F(ostringstream_int_99999)                                                 \
    F(ostringstream_double_99999)                                              \
    F(ostringstream_double_033333)                                             \
    F(alloc_istringstream)                                                     \
    F(istringstream_int_99999)                                                 \
    F(istringstream_double_99999)                                              \
    F(iomanip_set_precision)                                                   \
    F(alloc_string)                                                            \
    F(concat_hello_world)                                                      \
    F(new_delete_1byte)                                                        \
    F(unique_ptr_reset)                                                        \
    F(shared_ptr_incdec)                                                       \
    F(map1M_insert_remove)                                                     \
    F(map4_insert_remove)                                                      \
    F(sort1K)                                                                  \
    F(sort4)                                                                   \
    F(minstd_rand)                                                             \
    F(mt19937_rand)                                                            \
    F(default_rand)                                                            \
    F(stable_sort1K)                                                           \
    F(stable_sort4)                                                            \
    F(steady_clock_now)                                                        \
    F(system_clock_now)                                                        \
    F(steady_timepoint_to_sec)                                                 \
    F(variant_get)                                                             \
    F(typeid_get)

#define FOR_EACH_TEST(F)                                                       \
    FOR_EACH_TEST_GENERIC(F)                                                   \
    FOR_EACH_TEST_THREAD(F)                                                    \
    FOR_EACH_TEST_EXCEPTIONS(F)

struct LIBCXX : public BenchDesc {
    LIBCXX() : BenchDesc("libc++") {}

    virtual result_t run(GlobalState const *g) override {
        int count = 0;
#define INC_COUNT(F) count++;
        FOR_EACH_TEST(INC_COUNT);

        typedef Table1D<double, std::string> result_t;
        result_t *result = new result_t("test_name", count);

#define NAME(F) #F,

        result->column_label = " nsec/call";
        result->row_label = std::vector<std::string>{FOR_EACH_TEST(NAME)};

        count = 0;

#define RUN(F)                                                                 \
    {                                                                          \
        F t;                                                                   \
        (*result)[count++] = run_test_g(g, &t);                                \
    }
        FOR_EACH_TEST(RUN)

        return std::unique_ptr<BenchResult>(result);
    }
    virtual result_t parse_json_result(picojson::value const &v) override {
        return result_t(Table1D<double, std::string>::parse_json_result(v));
    }
};

} // namespace

std::unique_ptr<BenchDesc> get_libcxx_desc() {
    return std::unique_ptr<BenchDesc>(new LIBCXX());
}

} // namespace smbm
