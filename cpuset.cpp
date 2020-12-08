#include "cpuset.h"
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>

namespace smbm {

CPUSet::CPUSet() {
    this->ncpu_all = sysconf(_SC_NPROCESSORS_CONF);
    this->online_set = CPU_ALLOC(this->ncpu_all);

    sched_getaffinity(0, CPU_ALLOC_SIZE(this->ncpu_all), this->online_set);
}

CPUSet::~CPUSet() {
    CPU_FREE(this->online_set);
}

int CPUSet::first_cpu_pos() {
    for (int i=0; i<this->ncpu_all; i++) {
        if (CPU_ISSET_S(i, CPU_ALLOC_SIZE(this->ncpu_all), this->online_set)) {
            return i;
        }
    }

    abort();
}

int CPUSet::next_cpu_pos(int cur0)
{
    int cur = cur0;
    while (1) {
        int next = (cur + 1) % this->ncpu_all;
        if (CPU_ISSET_S(next, CPU_ALLOC_SIZE(this->ncpu_all), this->online_set)) {
            return next;
        }

        if (next == cur0) {
            break;
        }
    }

    abort();
}

void
CPUSet::set_affinity_self(int pos)
{
    cpu_set_t *set = CPU_ALLOC(this->ncpu_all);

    CPU_ZERO_S(CPU_ALLOC_SIZE(this->ncpu_all), set);
    CPU_SET_S(pos, CPU_ALLOC_SIZE(this->ncpu_all), set);

    sched_setaffinity(0, CPU_ALLOC_SIZE(this->ncpu_all), set);

    CPU_FREE(set);
}

}