#include "sys-microbenchmark.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <string.h>

namespace smbm {
std::vector< std::unique_ptr<BenchDesc> > get_benchmark_list() {
    std::vector< std::unique_ptr<BenchDesc> > ret;

#define F(B) ret.push_back(get_##B##_desc());

    FOR_EACH_BENCHMARK_LIST(F);

    return ret;
}

static int
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags )
{
    int ret;

    ret = syscall( __NR_perf_event_open, hw_event, pid, cpu,
                   group_fd, flags );
    return ret;
}



perf_counter::perf_counter() {
    struct perf_event_attr attr;
    memset(&attr, 0, sizeof(attr));

    attr.type = PERF_TYPE_HARDWARE;
    attr.size = sizeof(attr);
    attr.config = PERF_COUNT_HW_CPU_CYCLES;
    attr.exclude_kernel = 1;

    this->fd = perf_event_open(&attr, 0, -1, -1, 0);

    if (fd == -1) {
        perror("perf_vent_open");
        fprintf(stderr,
                "note : echo -1 > /proc/sys/kernel/perf_event_paranoid\n");
        exit(1);
    }
}

perf_counter::~perf_counter() { close(this->fd); }

uint64_t perf_counter::cpu_cycles() {
    long long val;
    ssize_t sz = read(this->fd, &val, sizeof(val));
    if (sz != sizeof(val)) {
        perror("read");
        exit(1);
    }

    return val;
}

} // namespace smbm