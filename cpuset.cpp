#include "cpuset.h"
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>

namespace smbm {

CPUSet::CPUSet() {}

CPUSet::~CPUSet() { CPU_FREE(this->online_set); }

CPUSet CPUSet::current_all_online() {
    CPUSet ret;
    ret.ncpu_all = sysconf(_SC_NPROCESSORS_CONF);
    ret.online_set = CPU_ALLOC(ret.ncpu_all);

    sched_getaffinity(0, ret.ss(), ret.online_set);

    return ret;
}

int CPUSet::first_cpu_pos() const {
    for (int i = 0; i < this->ncpu_all; i++) {
        if (CPU_ISSET_S(i, this->ss(), this->online_set)) {
            return i;
        }
    }

    abort();
}

int CPUSet::next_cpu_pos(int cur0) const {
    int cur = cur0;
    while (1) {
        int next = (cur + 1) % this->ncpu_all;
        if (CPU_ISSET_S(next, this->ss(), this->online_set)) {
            return next;
        }

        if (next == cur0) {
            break;
        }
    }

    abort();
}

ScopedSetAffinity ScopedSetAffinity::bind_self_to_1proc(CPUSet const *all) {
    cpu_set_t *old = CPU_ALLOC(all->ncpu_all);
    sched_getaffinity(0, all->ss(), old);

    cpu_set_t *set = CPU_ALLOC(all->ncpu_all);
    int pos = all->first_cpu_pos();

    CPU_ZERO_S(all->ss(), set);
    CPU_SET_S(pos, all->ss(), set);

    sched_setaffinity(0, all->ss(), set);

    CPU_FREE(set);

    return ScopedSetAffinity(old);
}

ScopedSetAffinity ScopedSetAffinity::bind_self_to_all(CPUSet const *all) {
    cpu_set_t *old = CPU_ALLOC(all->ncpu_all);
    sched_getaffinity(0, all->ss(), old);

    sched_setaffinity(0, all->ss(), all->online_set);

    return ScopedSetAffinity(old);
}

ScopedSetAffinity::~ScopedSetAffinity() { CPU_FREE(this->old); }

} // namespace smbm