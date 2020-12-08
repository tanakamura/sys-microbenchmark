#pragma once
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#if (defined __i386__) || (defined __x86_64__)
#define X86
#include <cpuid.h>
#include <immintrin.h>
#include <x86intrin.h>
#endif

#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define HAVE_HW_CPUCYCLE
#define HAVE_CLOCK_GETTIME
#include <time.h>
#endif

#if (defined X86) || (defined __aarch64__)
#define HAVE_USERLAND_CPUCOUNTER
#endif

#ifndef HAVE_USERLAND_CPUCOUNTER
#define USE_OS_TIMECOUNTER
#endif

#include "cpuset.h"
#include "picojson.h"

namespace smbm {

constexpr int PRINT_DOUBLE_PRECISION = 5;

enum class ResultValueUnit {
    CPU_HW_CYCLES,
    REALTIME_NSEC,
    BYTES_PER_SEC,
    CALL_PER_SEC
};

enum class Timer { POSIX_CLOCK_GETTIME, CPU_COUNTER };

struct GlobalState;

struct BenchResult {
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
    F(memory_bandwidth)                                                        \
    F(memory_latency)

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
        if (this->tv.tv_sec < r.tv.tv_nsec) {
            return false;
        }

        if (this->tv.tv_sec > r.tv.tv_nsec) {
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
};

struct userland_timer_value {
#if (defined X86) || (defined __aarch64__)
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
    inc_sec_userland_timer(struct userland_timer_value const *t, double sec);
    GlobalState(bool use_cpu_cycle_counter);
    ~GlobalState();
    double userland_timer_delta_to_sec(uint64_t delta);

#ifdef HAVE_HW_CPUCYCLE
    bool use_cpu_cycle_counter;

    uint64_t get_hw_cpucycle();

    cpu_dt_value get_cputime() {
        cpu_dt_value ret;
        if (use_cpu_cycle_counter) {
            ret.hw_cpu_cycle = get_hw_cpucycle();
        } else {
            ret.tv = userland_timer_value::get();
        }
        return ret;
    }

    ResultValueUnit get_cputime_unit() {
        if (use_cpu_cycle_counter) {
            return ResultValueUnit::CPU_HW_CYCLES;
        } else {
            return ResultValueUnit::REALTIME_NSEC;
        }
    }

    double delta_cputime(cpu_dt_value const *l, cpu_dt_value const *r);
#else

    cpu_timer get_cputime() {
        cpu_timer ret;
        ret.tv = userland_timer_value::get();
        return ret;
    }

    ResultValueUnit get_cputime_unit() {
        return ResultValueUnit::REALTIME_NSEC;
    }

    double delta_cputime(cpu_dt_value const *l, cpu_dt_value const *r) {
        return userland_timer_value_to_nsec(l->tv - r->tv);
    }

#endif

    uint64_t ostimer_delta_to_nsec_scale() { return 1; }
};

} // namespace smbm
