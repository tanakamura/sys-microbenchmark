#include "cpuset.h"
#include <memory>
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>

namespace smbm {

#ifdef HAVE_HWLOC

namespace {
struct count_node {
    bool finished;
    int count;
    hwloc_obj_t obj;

    std::vector<std::unique_ptr<count_node>> children;
    count_node(hwloc_obj_t obj, int nchild) : obj(obj), children(nchild) {}

    template <typename F> void map_before(F const &f) {
        f(*this);

        for (auto &&n : children) {
            n->map_before(f);
        }
    }

    hwloc_obj_t get_current_leaf() {
        if (children.size() == 0) {
            this->finished = true;
            return this->obj;
        } else {
            return children[count]->get_current_leaf();
        }
    }

    //    void dump(int indent) {
    //        for (int i=0; i<indent; i++) {
    //            printf(" ");
    //        }
    //        printf("%p : %s (%s), arity=%d, osidx=%d, count=%d,
    //        finished=%d\n", obj, obj->name, hwloc_obj_type_string(obj->type),
    //        obj->arity, obj->os_index, count, finished);
    //
    //        for (auto &&n : children) {
    //            n->dump(indent + 8);
    //        }
    //    }

    void inc() {
        if (children.size() == 0) {
            this->finished = true;
            return;
        }

        int start_count = this->count;
        int cur_count = this->count;

        children[cur_count]->inc();

        while (1) {
            cur_count = (cur_count + 1) % children.size();

            if (!children[cur_count]->finished) {
                break;
            }

            if (cur_count == start_count) {
                this->finished = true;
                break;
            }
        }

        this->count = cur_count;

        return;
    }
};

static void build_count_tree(hwloc_obj_t node,
                             std::unique_ptr<count_node> &loc) {
    // printf("%p : %s (%s), arity=%d, osidx=%d\n", node, node->name,
    // hwloc_obj_type_string(node->type), node->arity, node->os_index);

    loc = std::make_unique<count_node>(node, node->arity);

    if (node->arity == 0) {
        return;
    }

    for (int i = 0; i < (int)node->arity; i++) {
        build_count_tree(node->children[i], loc->children[i]);
    }
}

} // namespace

ProcessorTable::ProcessorTable() {
    int r = hwloc_topology_init(&this->topo);
    if (r == -1) {
        perror("hwloc_topolology_init");
        exit(1);
    }

    hwloc_topology_set_all_types_filter(this->topo, HWLOC_TYPE_FILTER_KEEP_ALL);
    hwloc_topology_set_io_types_filter(this->topo, HWLOC_TYPE_FILTER_KEEP_NONE);
    r = hwloc_topology_load(this->topo);
    if (r == -1) {
        perror("hwloc_topology_load");
        exit(1);
    }

    hwloc_obj_t root = hwloc_get_root_obj(this->topo);
    std::unique_ptr<count_node> croot;
    build_count_tree(root, croot);

    croot->map_before([this](count_node &n) {
        if (n.children.size() == 0) {
            this->table[PROC_ORDER_INNER_TO_OUTER].emplace_back(n.obj);
        }

        n.count = 0;
        n.finished = false;
    });

    while (!croot->finished) {
        hwloc_obj_t leaf = croot->get_current_leaf();
        this->table[PROC_ORDER_OUTER_TO_INNER].emplace_back(leaf);
        croot->inc();
    }

    this->startup_set = hwloc_bitmap_alloc();
    r = hwloc_get_cpubind(this->topo, this->startup_set, HWLOC_CPUBIND_THREAD);
    if (r < 0) {
        perror("hwloc_get_cpubind");
        exit(1);
    }
}

ProcessorTable::~ProcessorTable() {
    hwloc_topology_destroy(this->topo);
    hwloc_bitmap_free(this->startup_set);
}

void bind_self_to_1proc(std::unique_ptr<ProcessorTable> const &tbl,
                        ProcessorIndex idx, bool membind) {
    int r = hwloc_set_cpubind(tbl->topo, idx.pu_obj->cpuset, HWLOC_CPUBIND_THREAD);
    if (r < 0) {
        perror("set_cpubind");
        exit(1);
    }
    if (membind) {
        r = hwloc_set_membind(tbl->topo, idx.pu_obj->cpuset, HWLOC_MEMBIND_BIND,
                              HWLOC_MEMBIND_THREAD | HWLOC_MEMBIND_NOCPUBIND);
        if (r < 0) {
            perror("set_membind");
            exit(1);
        }
    }
}

void bind_self_to_first(std::unique_ptr<ProcessorTable> const &tbl,
                        bool membind) {
    ProcessorIndex idx = tbl->logical_index_to_processor(0, PROC_ORDER_OUTER_TO_INNER);
    bind_self_to_1proc(tbl, idx, membind);
}

void bind_self_to_all(std::unique_ptr<ProcessorTable> const &tbl) {
    int r = hwloc_set_cpubind(tbl->topo, tbl->startup_set, HWLOC_CPUBIND_THREAD|HWLOC_CPUBIND_NOMEMBIND);
    if (r < 0) {
        perror("set_cpubind");
        exit(1);
    }
}

#else
#error "affinity"
#endif

} // namespace smbm