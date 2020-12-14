#pragma once

#include <vector>
#include <memory>

#include "features.h"

#ifdef HAVE_HWLOC
#include <hwloc.h>
#endif


namespace smbm {

enum {
    PROC_ORDER_OUTER_TO_INNER,
    PROC_ORDER_INNER_TO_OUTER,

    NPROC_ORDER,
};

#ifdef HAVE_HWLOC
struct ProcessorIndex {
    hwloc_obj_t pu_obj;
    ProcessorIndex(hwloc_obj_t o)
        :pu_obj(o)
    {}
};

struct ProcessorTable {
    hwloc_topology_t topo;

    std::vector<ProcessorIndex> table[NPROC_ORDER];
    const ProcessorIndex logical_index_to_processor(int logical_index, int order) const {
        return table[order][logical_index];
    }

    int get_active_cpu_count() const {
        return (int)table[0].size();
    }

    ProcessorTable(const ProcessorTable &rhs)  = delete;
    void operator=(const ProcessorTable &) = delete;

    ProcessorTable();
    ~ProcessorTable();
};
#endif

void bind_self_to_1proc(std::unique_ptr<ProcessorTable> const &tbl, ProcessorIndex idx, bool membind);

} // namespace smbm
