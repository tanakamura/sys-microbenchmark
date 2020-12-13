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

#include "cpuset.h"
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
    virtual picojson::value dump_json() = 0;
    virtual void dump_human_readable(std::ostream &, int double_precision) = 0;
};

struct BenchDesc {
    std::string name;
    typedef std::unique_ptr<BenchResult> result_t;
    BenchDesc(std::string const &s) : name(s) {}
    virtual ~BenchDesc() {}
    virtual result_t run(GlobalState *g) = 0;
    virtual result_t parse_json_result(picojson::value const &v) = 0;
    virtual int double_precision() { return PRINT_DOUBLE_PRECISION; }
};

#define FOR_EACH_BENCHMARK_LIST(F)                                             \
    F(idiv32)                                                                  \
    F(idiv64)                                                                  \
    F(syscall)                                                                 \
    F(memory_bandwidth_1thread)                                                 \
    F(memory_bandwidth_full_thread)                                     \
    F(cache_bandwidth)                                                  \
    F(memory_random_access)

std::vector<std::unique_ptr<BenchDesc>> get_benchmark_list();

#define DEFINE_ENTRY(B) std::unique_ptr<BenchDesc> get_##B##_desc();

FOR_EACH_BENCHMARK_LIST(DEFINE_ENTRY)

#define REP8(A) A A A A A A A A
#define REP16(A) REP8(A) REP8(A)

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

    static const char *name() {
        return "clock_gettime";
    }
};

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
    static const char *name() {
        return "cntvct";
    }
#else
    static const char *name() {
        return "rdtscp";
    }
#endif

#else

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

    static const char *name = "ostimer";

#endif
};

union cpu_dt_value {
    struct userland_timer_value tv;
    uint64_t hw_cpu_cycle;
};

struct GlobalState {
    CPUSet cpus;

#ifdef HAVE_USERLAND_CPUCOUNTER
    double userland_cpucounter_freq;
#endif

#ifdef __linux__
    int perf_fd;
#endif

    userland_timer_value
    inc_sec_userland_timer(struct userland_timer_value const *t, double sec) const;
    GlobalState(bool use_cpu_cycle_counter);
    ~GlobalState();
    double userland_timer_delta_to_sec(uint64_t delta) const;

    uint64_t *zero_memory;
    uint64_t **dustbox;

    uint64_t getzero() const {
        return *zero_memory;
    };

    void dummy_write(int cpu, uint64_t val) const {
        dustbox[cpu][0] = val;
    };

#ifdef HAVE_HW_CPUCYCLE
    bool use_cpu_cycle_counter;

    uint64_t get_hw_cpucycle() const ;

    cpu_dt_value get_cputime() const {
        cpu_dt_value ret;
        if (use_cpu_cycle_counter) {
            ret.hw_cpu_cycle = get_hw_cpucycle();
        } else {
            ret.tv = userland_timer_value::get();
        }
        return ret;
    }

    ResultValueUnit get_cputime_unit() const {
        if (use_cpu_cycle_counter) {
            return ResultValueUnit::CPU_HW_CYCLES;
        } else {
            return ResultValueUnit::REALTIME_NSEC;
        }
    }

    double delta_cputime(cpu_dt_value const *l, cpu_dt_value const *r) const ;
#else

    cpu_timer get_cputime() const {
        cpu_timer ret;
        ret.tv = userland_timer_value::get();
        return ret;
    }

    ResultValueUnit get_cputime_unit() const {
        return ResultValueUnit::REALTIME_NSEC;
    }

    double delta_cputime(cpu_dt_value const *l, cpu_dt_value const *r) const {
        return userland_timer_value_to_nsec(l->tv - r->tv);
    }

#endif

    uint64_t ostimer_delta_to_nsec_scale() { return 1; }
};

void warmup_thread(GlobalState const *g);

} // namespace smbm
