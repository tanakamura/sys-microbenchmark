#pragma once

#include <sched.h>

namespace smbm {

struct CPUSet {
    int ncpu_all;

    cpu_set_t *online_set;

    CPUSet();
    ~CPUSet();

    int first_cpu_pos();
    int next_cpu_pos(int cur);

    void set_affinity_self(int pos);
};

} // namespace smbm
