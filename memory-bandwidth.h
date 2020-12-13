#include "cpu-feature.h"
#include "sys-microbenchmark.h"

#pragma once

namespace smbm {
typedef uint64_t (*load_func_t)(void const *src, size_t sz);

extern load_func_t get_architecture_load_func(GlobalState *g);
extern uint64_t simple_long_sum_test(void const *src, size_t sz);
extern uint64_t gccvec128_load_test(void const *src, size_t sz);

}


