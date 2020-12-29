#include "sys-microbenchmark.h"
#include "oneshot_timer.h"
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <sstream>

#ifdef __linux__
#include <linux/perf_event.h>
#include <sys/syscall.h>
#endif

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#ifdef HAVE_CPUINFO
#include <cpuinfo.h>
#endif

#include <math.h>
#include <string.h>
#include <unistd.h>

#include "cpuset.h"
#include "memalloc.h"

namespace smbm {
#ifdef __linux__
static int perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu,
                           int group_fd, unsigned long flags) {
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
    return ret;
}
#endif

static double ooo_check() {
    int data = 0;
    int n = 1024 * 1024;
    asm volatile(" " : "+r"(data), "+r"(n));

    double delta0, delta1;

    {
        int sum0 = 0, sum1 = 0;

        auto t0 = userland_timer_value::get();
        for (int i = 0; i < n; i++) {
            sum0 *= data;
            asm volatile(" " : "+r"(sum0));
            sum0 *= data;
            asm volatile(" " : "+r"(sum0));
            sum0 *= data;
            asm volatile(" " : "+r"(sum0));
            sum0 *= data;
            asm volatile(" " : "+r"(sum0));

            sum1 *= data;
            asm volatile(" " : "+r"(sum1));
            sum1 *= data;
            asm volatile(" " : "+r"(sum1));
            sum1 *= data;
            asm volatile(" " : "+r"(sum1));
            sum1 *= data;
            asm volatile(" " : "+r"(sum1));
        }
        auto t1 = userland_timer_value::get();

        delta0 = t1 - t0;

        int sum2 = sum0 + sum1;
        asm volatile(" " : : "r"(sum2));
    }

    {
        int sum0 = 0, sum1 = 0;

        auto t0 = userland_timer_value::get();
        for (int i = 0; i < n; i++) {
            sum0 *= data;
            asm volatile(" " : "+r"(sum0));
            sum1 *= data;
            asm volatile(" " : "+r"(sum1));

            sum0 *= data;
            asm volatile(" " : "+r"(sum0));
            sum1 *= data;
            asm volatile(" " : "+r"(sum1));

            sum0 *= data;
            asm volatile(" " : "+r"(sum0));
            sum1 *= data;
            asm volatile(" " : "+r"(sum1));

            sum0 *= data;
            asm volatile(" " : "+r"(sum0));
            sum1 *= data;
            asm volatile(" " : "+r"(sum1));
        }
        auto t1 = userland_timer_value::get();

        delta1 = t1 - t0;

        int sum2 = sum0 + sum1;
        asm volatile(" " : : "r"(sum2));
    }

    double ratio = delta0 / delta1;
    printf("ooo ratio : %f\n", ratio);

    return ratio;
}

static inline void ostimer_delay_loop(GlobalState *g, double sec) {
    uint64_t target_delta = g->sec_to_ostimer_delta(sec);

    auto t0 = ostimer_value::get();
    while (1) {
        auto t1 = ostimer_value::get();

        uint64_t d = t1 - t0;

        if (d >= target_delta) {
            break;
        }
    }
}

GlobalState::GlobalState() : proc_table(new ProcessorTable()) {
#ifdef HAVE_CPUINFO
    cpuinfo_initialize();
#endif

    void *p = aligned_calloc(64, 64);

    this->zero_memory = (uint64_t *)p;
    *this->zero_memory = 0;

    int ncpu = proc_table->get_active_cpu_count();
    this->dustbox = new uint64_t *[ncpu];

    for (int i = 0; i < ncpu; i++) {
        p = aligned_calloc(64, 64);
        this->dustbox[i] = (uint64_t *)p;
    }

    bind_self_to_1proc(this->proc_table,
                       this->proc_table->logical_index_to_processor(
                           0, PROC_ORDER_OUTER_TO_INNER),
                       true);

#ifdef WINDOWS
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        this->ostimer_freq = freq.QuadPart;
    }
#endif

#ifdef HAVE_USERLAND_CPUCOUNTER
    {
        double measure_delay_sec = 0.05;
        double delay_to_sec = 1 / measure_delay_sec;

        uint64_t min = -(1ULL);

        for (int i = 0; i < 5; i++) {
            auto t0 = userland_timer_value::get();
            ostimer_delay_loop(this, measure_delay_sec);
            auto t1 = userland_timer_value::get();

            uint64_t d = t1 - t0;
            min = std::min(min, d);
        }

        uint64_t freq = min * delay_to_sec;

        this->userland_cpucounter_freq = freq;
    }
#endif

    this->ooo_ratio = ooo_check();

#ifdef __linux__

    {
        struct perf_event_attr attr;
        memset(&attr, 0, sizeof(attr));

        attr.type = PERF_TYPE_HARDWARE;
        attr.size = sizeof(attr);
        attr.config = PERF_COUNT_HW_CPU_CYCLES;
        attr.exclude_kernel = 1;

        this->perf_fd_cycle = perf_event_open(&attr, 0, -1, -1, 0);

        if (this->perf_fd_cycle == -1) {
            this->hw_perf_counter_available = false;

            perror("perf_event_open");
            fprintf(stderr, "note : to enable perf counter, run 'echo -1 > "
                            "/proc/sys/kernel/perf_event_paranoid' in shell\n");
        } else {
            this->hw_perf_counter_available = true;

            memset(&attr, 0, sizeof(attr));
            attr.type = PERF_TYPE_HARDWARE;
            attr.size = sizeof(attr);
            attr.config = PERF_COUNT_HW_BRANCH_MISSES;
            attr.exclude_kernel = 1;

            this->perf_fd_branch = perf_event_open(&attr, 0, -1, -1, 0);
        }
    }

#endif

#define F(B)                                                                   \
    {                                                                          \
        auto x = std::shared_ptr<BenchDesc>(get_##B##_desc());                 \
        if (x && x->available(this)) {                                         \
            this->bench_list.push_back(x);                                     \
        }                                                                      \
    };
    FOR_EACH_BENCHMARK_LIST(F);
}

std::vector<std::shared_ptr<BenchDesc>> get_all_benchmark_list() {
    std::vector<std::shared_ptr<BenchDesc>> ret;

#define FA(B)                                                                  \
    {                                                                          \
        auto x = std::shared_ptr<BenchDesc>(get_##B##_desc());                 \
        ret.push_back(x);                                                      \
    };                                                                         \

    FOR_EACH_BENCHMARK_LIST(FA);

    return ret;
}

double GlobalState::userland_timer_delta_to_sec(uint64_t delta) const {
#ifdef HAVE_USERLAND_CPUCOUNTER
    return delta / this->userland_cpucounter_freq;
#elif defined HAVE_CLOCK_GETTIME
    return delta / 1e9;
#elif defined USE_OSTIMER_AS_USERLAND_TIMER
    return ostimer_delta_to_sec(delta);
#else
#error "xx"
#endif
}

userland_timer_value
GlobalState::inc_sec_userland_timer(userland_timer_value const *t0,
                                    double sec) const {
#ifdef HAVE_USERLAND_CPUCOUNTER
    userland_timer_value t1;
    t1.v64 = t0->v64 + (uint64_t)(sec * this->userland_cpucounter_freq);
    return t1;
#elif defined HAVE_CLOCK_GETTIME
    uint64_t isec = t0->v.tv.tv_sec + (uint64_t)floor(sec);
    uint64_t insec = t0->v.tv.tv_nsec + (uint64_t)((fmod(sec, 1.0)) * 1e9);

    if (insec >= 1000000000) {
        isec++;
        insec -= 1000000000;
    }

    userland_timer_value t1;
    t1.v.tv.tv_sec = isec;
    t1.v.tv.tv_nsec = insec;
    return t1;
#elif defined EMSCRIPTEN

    userland_timer_value t1;
    t1.v.v = t0->v.v + GlobalState::ostimer_freq;
    return t1;

#else
#error "xx"
#endif
}

GlobalState::~GlobalState() {
    aligned_free(this->zero_memory);

    int ncpu = proc_table->get_active_cpu_count();

    for (int i = 0; i < ncpu; i++) {
        aligned_free(this->dustbox[i]);
    }

    delete[] this->dustbox;

#ifdef __linux__
    if (this->hw_perf_counter_available) {
        close(this->perf_fd_cycle);
        close(this->perf_fd_branch);
    }
#endif

#ifdef HAVE_CPUINFO
    cpuinfo_deinitialize();
#endif
}

#ifdef __linux__
uint64_t GlobalState::get_hw_cpucycle() const {
    long long val;
    ssize_t sz = read(this->perf_fd_cycle, &val, sizeof(val));
    if (sz != sizeof(val)) {
        perror("read");
        exit(1);
    }

    return val;
}
uint64_t GlobalState::get_hw_branch_miss() const {
    long long val;
    ssize_t sz = read(this->perf_fd_branch, &val, sizeof(val));
    if (sz != sizeof(val)) {
        perror("read");
        exit(1);
    }

    return val;
}

#endif

void warmup_thread(const GlobalState *g) {
    oneshot_timer ot(1024);

    ot.start(g, 1.0);

    volatile int counter = 0;
    while (!ot.test_end()) {
        counter++;
    }
}

std::string byte1024(size_t sz, int prec) {
    std::ostringstream oss;

    oss << std::fixed << std::setprecision(prec);
    if (sz <= 1024) {
        oss << sz;
        oss << "[Bytes]";
    } else if (sz <= (1024 * 1024)) {
        oss << (sz / (1024.0));
        oss << "[KiB]";
    } else if (sz <= (1024 * 1024 * 1024)) {
        oss << (sz / (1024.0 * 1024.0));
        oss << "[MiB]";
    } else {
        oss << (sz / (1024.0 * 1024.0 * 1024.0));
        oss << "[GiB]";
    }

    return oss.str();
}

#ifdef EMSCRIPTEN

EM_JS(double, get_js_tick, (), { return performance.now(); });

#endif

} // namespace smbm
