#include "cpu-feature.h"
#include "sys-microbenchmark.h"

#pragma once

namespace smbm {
typedef uint64_t (*load_func_t)(void const *src, size_t sz);

extern uint64_t simple_long_sum_test(void const *src, size_t sz);
extern uint64_t gccvec128_load_test(void const *src, size_t sz);
extern void gccvec128_copy_test(void *dst, void const *src, size_t sz);
extern void gccvec128_store_test(void *dst, size_t sz);

struct MemFuncs {
    void (*p_copy_fn)(void *dst, void const *src, size_t sz);
    void (*p_store_fn)(void *dst, size_t sz);
    uint64_t (*p_load_fn)(void const *src, size_t sz);
};

extern MemFuncs get_fastest_memfunc(GlobalState const *g);

}


