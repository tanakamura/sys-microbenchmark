#include "sys-microbenchmark.h"
#include <fcntl.h>
#include <linux/hw_breakpoint.h>
#include <linux/perf_event.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <math.h>

namespace smbm {
std::vector<std::unique_ptr<BenchDesc>> get_benchmark_list() {
    std::vector<std::unique_ptr<BenchDesc>> ret;

#define F(B) ret.push_back(get_##B##_desc());

    FOR_EACH_BENCHMARK_LIST(F);

    return ret;
}

#ifdef __linux__
static int perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu,
                           int group_fd, unsigned long flags) {
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
    return ret;
}
#endif

static inline void
ostimer_delay_loop(GlobalState *g, uint64_t msec) {
    uint64_t target_delta = (msec*1000000) * g->ostimer_delta_to_nsec_scale();

    auto t0 = ostimer_value::get();
    while (1) {
        auto t1 = ostimer_value::get();

        uint64_t d = t1-t0;

        if (d >= target_delta) {
            break;
        }
    }
}


GlobalState::GlobalState(bool use_cpu_cycle_counter)
    :cpus(CPUSet::current_all_online())
{
    void *p = 0;
    posix_memalign(&p, 64, 64);

    this->zero_memory = (uint64_t*)p;
    *this->zero_memory = 0;

    int ncpu = cpus.ncpu_all;
    this->dustbox = new uint64_t*[ncpu];

    for (int i=0; i<ncpu; i++) {
        posix_memalign(&p, 64, 64);
        this->dustbox[i] = (uint64_t*)p;
    }

#ifdef HAVE_USERLAND_CPUCOUNTER
    {
        uint64_t measure_delay = 50;
        uint64_t delay_to_sec = 1000 / measure_delay;

        uint64_t min = -(1ULL);

        for (int i=0; i<5; i++) {
            auto t0 = userland_timer_value::get();
            ostimer_delay_loop(this, measure_delay);
            auto t1 = userland_timer_value::get();

            uint64_t d = t1 - t0;
            min = std::min(min, d);
        }

        uint64_t freq = min * delay_to_sec;

        this->userland_cpucounter_freq = freq;
    }
#endif


#ifdef __linux__
    this->use_cpu_cycle_counter = use_cpu_cycle_counter;
    if (use_cpu_cycle_counter) {
        struct perf_event_attr attr;
        memset(&attr, 0, sizeof(attr));

        attr.type = PERF_TYPE_HARDWARE;
        attr.size = sizeof(attr);
        attr.config = PERF_COUNT_HW_CPU_CYCLES;
        attr.exclude_kernel = 1;

        this->perf_fd = perf_event_open(&attr, 0, -1, -1, 0);

        if (this->perf_fd == -1) {
            perror("perf_vent_open");
            fprintf(stderr,
                    "note : echo -1 > /proc/sys/kernel/perf_event_paranoid\n");
            exit(1);
        }
    }
#endif
}

double
GlobalState::userland_timer_delta_to_sec(uint64_t delta) {
#ifdef HAVE_USERLAND_CPUCOUNTER
    return delta / this->userland_cpucounter_freq;
#elif defined HAVE_CLOCK_GETTIME
    return delta / 1e9;
#else
#error "xx"
#endif
}

userland_timer_value
GlobalState::inc_sec_userland_timer(userland_timer_value const *t0, double sec)
{
#ifdef HAVE_USERLAND_CPUCOUNTER
    userland_timer_value t1;
    t1.v64 = t0->v64 + (uint64_t)(sec * this->userland_cpucounter_freq);
    return t1;
#elif defined HAVE_CLOCK_GETTIME
    uint64_t isec = t0->v.tv.tv_sec + (uint64_t)floor(sec);
    uint64_t insec = t0->v.tv.tv_nsec + (uint64_t)((fmod(sec,1.0)) * 1e9);

    if (insec >= 1000000000) {
        isec++;
        insec -= 1000000000;
    }

    userland_timer_value t1;
    t1.v.tv.tv_sec = isec;
    t1.v.tv.tv_nsec = insec;
    return t1;
#else
#error "xx"
#endif
    
}

GlobalState::~GlobalState() {
#ifdef __linux__
    if (this->use_cpu_cycle_counter) {
        close(this->perf_fd);
    }
#endif
}

uint64_t GlobalState::get_hw_cpucycle() {
    long long val;
    ssize_t sz = read(this->perf_fd, &val, sizeof(val));
    if (sz != sizeof(val)) {
        perror("read");
        exit(1);
    }

    return val;
}

double GlobalState::delta_cputime(cpu_dt_value const *l, cpu_dt_value const *r)
{
    if (this->use_cpu_cycle_counter) {
        return l->hw_cpu_cycle - r->hw_cpu_cycle;
    } else {
        return userland_timer_delta_to_sec(l->tv - r->tv) * 1e9; // nsec
    }
}


} // namespace smbm
