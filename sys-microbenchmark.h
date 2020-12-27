#pragma once
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "features.h"

#ifdef X86
#include <x86intrin.h>
#endif

#include "picojson.h"

namespace smbm {

constexpr int PRINT_DOUBLE_PRECISION = 5;

enum class ResultValueUnit {
    CPU_HW_CYCLES,
    REALTIME_NSEC,
    BYTES_PER_SEC,
    MiBYTES_PER_SEC,
    CALL_PER_SEC
};

enum class Timer { POSIX_CLOCK_GETTIME, CPU_COUNTER };

struct GlobalState;

struct BenchResult {
    virtual ~BenchResult() {}
    virtual picojson::value dump_json() const = 0;
    virtual void dump_human_readable(std::ostream &, int double_precision) = 0;
};

struct BenchDesc {
    std::string name;
    typedef std::unique_ptr<BenchResult> result_t;
    BenchDesc(std::string const &s) : name(s) {}
    virtual ~BenchDesc() {}
    virtual result_t run(GlobalState const *g) = 0;
    virtual result_t parse_json_result(picojson::value const &v) = 0;
    virtual int double_precision() { return PRINT_DOUBLE_PRECISION; }
    virtual bool available(GlobalState const *g) { return true; }
};

#define FOR_EACH_BENCHMARK_LIST(F)                                             \
    F(idiv32)                                                                  \
    F(idiv64)                                                                  \
    F(idiv32_cycle)                                                            \
    F(idiv64_cycle)                                                            \
    F(syscall)                                                                 \
    F(memory_bandwidth_1thread)                                                \
    F(memory_bandwidth_full_thread)                                            \
    F(cache_bandwidth_1thread)                                                 \
    F(cache_bandwidth_full_thread)                                             \
    F(memory_random_access_seq)                                                \
    F(memory_random_access_para)                                               \
    F(openmp)                                                                  \
    F(actual_freq)                                                             \
    F(inter_processor_communication)                                           \
    F(inter_processor_communication_yield)                                     \
    F(random_branch)                                                           \
    F(inst_random_branch)                                                      \
    F(iter_random_branch)                                                      \
    F(cos_branch)                                                              \
    F(indirect_branch)                                                         \
    F(random_branch_hit)                                                       \
    F(inst_random_branch_hit)                                                  \
    F(iter_random_branch_hit)                                                  \
    F(cos_branch_hit)                                                          \
    F(indirect_branch_hit)                                                     \
    F(instructions)                                                            \
    F(pipe)                                                                    \
    F(libc)                                                                    \
    F(libcxx)

#define DEFINE_ENTRY(B) std::unique_ptr<BenchDesc> get_##B##_desc();

FOR_EACH_BENCHMARK_LIST(DEFINE_ENTRY)

#define REP8(A) A A A A A A A A
#define REP16(A) REP8(A) REP8(A)

#ifdef HAVE_CLOCK_GETTIME

inline uint64_t delta_timespec(struct timespec const *l,
                               struct timespec const *r) {
    uint64_t delta_sec = (l->tv_sec - r->tv_sec);

    int64_t tl_nsec = l->tv_nsec;
    if (r->tv_nsec > tl_nsec) {
        delta_sec--;
        tl_nsec += 1000000000;
    }
    uint64_t delta_nsec = tl_nsec - r->tv_nsec;

    return delta_sec * 1000000000ULL + delta_nsec;
}

struct ostimer_value {
    struct timespec tv;

    bool operator>=(struct ostimer_value const &r) const {
        if (this->tv.tv_sec < r.tv.tv_sec) {
            return false;
        }

        if (this->tv.tv_sec > r.tv.tv_sec) {
            return true;
        }

        return this->tv.tv_nsec >= r.tv.tv_nsec;
    }

    uint64_t operator-(struct ostimer_value const &r) const {
        return delta_timespec(&this->tv, &r.tv);
    }

    static ostimer_value get() {
        ostimer_value ret;
        clock_gettime(CLOCK_MONOTONIC, &ret.tv);
        return ret;
    }

    static const char *name() { return "clock_gettime"; }
};
#elif defined WINDOWS

struct ostimer_value {
    LARGE_INTEGER v;

    bool operator>=(struct ostimer_value const &r) const {
        return this->v.QuadPart >= r.v.QuadPart;
    }
    uint64_t operator-(struct ostimer_value const &r) const {
        return this->v.QuadPart - r.v.QuadPart;
    }

    static ostimer_value get() {
        ostimer_value ret;
        QueryPerformanceCounter(&ret.v);
        return ret;
    }

    static const char *name() { return "QueryPerformanceCounter"; }
};

#elif (defined EMSCRIPTEN)

extern "C" double get_js_tick();

struct ostimer_value {
    double v;

    bool operator>=(ostimer_value const &r) const { return this->v >= r.v; }

    uint64_t operator-(ostimer_value const &r) const {
        return (uint64_t)(((this->v - r.v) * 1000.0) + 0.5);
    }

    static ostimer_value get() {
        double v = get_js_tick();
        ostimer_value ret;
        ret.v = v;
        return ret;
    }

    static const char *name() { return "w3c_Performance.now"; }
};

#else
#error "ostimer"
#endif

struct userland_timer_value {
#ifdef HAVE_USERLAND_CPUCOUNTER
    uint64_t v64;
    bool operator>=(struct userland_timer_value const &r) const {
        return this->v64 >= r.v64;
    }

    uint64_t operator-(struct userland_timer_value const &r) const {
        return this->v64 - r.v64;
    }

    static userland_timer_value get() {
        userland_timer_value ret;
        uint64_t v64;
#ifdef __aarch64__
        asm volatile("mrs %0, cntvct_el0" : "=r"(v64));
#else
        unsigned int z;
        v64 = __rdtscp(&z);
#endif
        ret.v64 = v64;
        return ret;
    }

#ifdef __aarch64__
    static const char *name() { return "cntvct"; }
#else
    static const char *name() { return "rdtscp"; }
#endif

#else

#define USE_OSTIMER_AS_USERLAND_TIMER

    struct ostimer_value v;

    bool operator>=(struct userland_timer_value const &r) const {
        return this->v >= r.v;
    }
    uint64_t operator-(struct userland_timer_value const &r) const {
        return this->v - r.v;
    }

    static userland_timer_value get() {
        userland_timer_value ret;
        ret.v = ostimer_value::get();
        return ret;
    }

    static const char *name() { return "ostimer"; };
#endif
};

typedef uint64_t perf_counter_value_t;
struct ProcessorTable;

struct GlobalState {
    std::unique_ptr<ProcessorTable> proc_table;

    std::vector<std::shared_ptr<BenchDesc>> bench_list;
    std::vector<std::shared_ptr<BenchDesc>> get_benchmark_list() {
        return bench_list;
    }

#ifdef HAVE_USERLAND_CPUCOUNTER
    double userland_cpucounter_freq;
#endif

#ifdef HAVE_HW_PERF_COUNTER
    int perf_fd_cycle;
    int perf_fd_branch;
    bool hw_perf_counter_available;
    bool is_hw_perf_counter_available() const {
        return hw_perf_counter_available;
    }
#else
    bool is_hw_perf_counter_available() const { return false; }
#endif

    userland_timer_value
    inc_sec_userland_timer(struct userland_timer_value const *t,
                           double sec) const;
    GlobalState();
    ~GlobalState();
    double userland_timer_delta_to_sec(uint64_t delta) const;

    uint64_t *zero_memory;
    uint64_t **dustbox;

    uint64_t getzero() const { return *zero_memory; };

    void dummy_write(int cpu, uint64_t val) const { dustbox[cpu][0] = val; };

#ifdef HAVE_HW_PERF_COUNTER
    perf_counter_value_t get_hw_cpucycle() const;
    perf_counter_value_t get_hw_branch_miss() const;
    perf_counter_value_t get_hw_cache_miss() const;
#endif

#ifdef HAVE_CLOCK_GETTIME
    /* timespec hold ns value */
    static constexpr double ostimer_freq = 1e9;
#endif

#ifdef WINDOWS
    double ostimer_freq;
#endif

#ifdef EMSCRIPTEN
    /* usec */
    static constexpr double ostimer_freq = 1e6;
#endif

    double ostimer_delta_to_sec(uint64_t delta) const {
        return delta / ostimer_freq;
    }
    uint64_t sec_to_ostimer_delta(double sec) const {
        return (uint64_t)((sec * ostimer_freq) + 0.5);
    }
};

void warmup_thread(GlobalState const *g);

std::string byte1024(size_t sz, int prec);

} // namespace smbm
