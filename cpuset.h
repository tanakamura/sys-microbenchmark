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

#ifdef HAVE_THREAD

#ifdef HAVE_HWLOC
struct ProcessorIndex {
    hwloc_obj_t pu_obj;
    ProcessorIndex(hwloc_obj_t o)
        :pu_obj(o)
    {}
};

struct ProcessorTable {
    hwloc_topology_t topo;
    hwloc_cpuset_t startup_set;

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

#else
#error "cpuset is not implemented"
#endif

void bind_self_to_1proc(std::unique_ptr<ProcessorTable> const &tbl, ProcessorIndex idx, bool membind);
void bind_self_to_first(std::unique_ptr<ProcessorTable> const &tbl, bool membind);

void bind_self_to_all(std::unique_ptr<ProcessorTable> const &tbl);

#else

struct ProcessorIndex {
};

struct ProcessorTable {
    const ProcessorIndex logical_index_to_processor(int logical_index, int order) const {
        return ProcessorIndex();
    }

    int get_active_cpu_count() const {
        return 1;
    }

    ProcessorTable(const ProcessorTable &rhs)  = delete;
    void operator=(const ProcessorTable &) = delete;

    ProcessorTable() {} 
    ~ProcessorTable() {}
};

inline void bind_self_to_1proc(std::unique_ptr<ProcessorTable> const &tbl, ProcessorIndex idx, bool membind) {}
inline void bind_self_to_first(std::unique_ptr<ProcessorTable> const &tbl, bool membind) {}
inline void bind_self_to_all(std::unique_ptr<ProcessorTable> const &tbl) {}

#endif

} // namespace smbm
