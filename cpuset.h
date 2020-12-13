#pragma once

#include <sched.h>
#include <vector>
#include "features.h"

namespace smbm {

#ifdef WINDOWS
#elif defined HAVE_GNU_CPU_SET
struct ProcessorTable {
    int ncpu_configured;
    cpu_set_t *set;

    std::vector<int> logical_idx_to_os_proc;

    int logical_idx2os_idx(int logical_index) const {
        return logical_idx_to_os_proc[logical_index];
    }

    int get_active_cpu_count() const {
        return logical_idx_to_os_proc.size();
    }

    size_t ss() const { 
        return CPU_ALLOC_SIZE(this->ncpu_configured);
    }

    ProcessorTable(const ProcessorTable &rhs)  = delete;
    void operator=(const ProcessorTable &) = delete;

    ProcessorTable();
    ~ProcessorTable() {
        CPU_FREE(set);
    }
};
#endif


void bind_self_to_1proc(ProcessorTable const *tbl, int idx);

} // namespace smbm
