#include "cpuset.h"
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>

namespace smbm {

ProcessorTable::ProcessorTable() {
    this->ncpu_configured = sysconf(_SC_NPROCESSORS_CONF);
    this->set = CPU_ALLOC(this->ss());

    sched_getaffinity(0, this->ss(), this->set);

    for (int i = 0; i < this->ncpu_configured; i++) {
        if (CPU_ISSET_S(i, this->ss(), this->set)) {
            this->logical_idx_to_os_proc.push_back(i);
        }
    }
}

#ifdef WINDOWS
#elif defined HAVE_GNU_CPU_SET
void bind_self_to_1proc(ProcessorTable const *tbl, int idx)
{
    int new_proc = tbl->logical_idx2os_idx(idx);
    cpu_set_t *set = CPU_ALLOC(tbl->ncpu_configured);
    
    CPU_ZERO_S(tbl->ss(), set);
    CPU_SET_S(new_proc, tbl->ss(), set);
    sched_setaffinity(0, tbl->ss(), set);

    CPU_FREE(set);
}
#else
#error "affinity"
#endif

} // namespace smbm