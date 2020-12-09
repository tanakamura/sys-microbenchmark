#pragma once

#include <sched.h>

namespace smbm {

struct CPUSet {
    int ncpu_all;
    cpu_set_t *online_set;

    int first_cpu_pos() const;
    int next_cpu_pos(int cur) const;

    ~CPUSet();

    void operator=(const CPUSet &) = delete;

    size_t ss() const { return CPU_ALLOC_SIZE(this->ncpu_all); }

    static CPUSet current_all_online();

  private:
    CPUSet(const CPUSet &) = default;
    CPUSet();
};

struct ScopedSetAffinity {
    cpu_set_t *old;

    void operator=(const ScopedSetAffinity &) = delete;

    ~ScopedSetAffinity();

    static ScopedSetAffinity bind_self_to_1proc(CPUSet const *all);
    static ScopedSetAffinity bind_self_to_all(CPUSet const *all);

  private:
    ScopedSetAffinity(const ScopedSetAffinity &) = default;
    ScopedSetAffinity(cpu_set_t *old) : old(old) {}
};

ScopedSetAffinity bind_self_to_1proc(CPUSet const *all);
ScopedSetAffinity bind_self_to_all(CPUSet const *all);

} // namespace smbm
