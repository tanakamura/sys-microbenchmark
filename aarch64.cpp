#include "features.h"
#include "memory-bandwidth.h"

#ifdef AARCH64

namespace smbm {
load_func_t get_architecture_load_func(GlobalState *g) {
    return gccvec128_load_test;
}
}

#endif