#include "features.h"

#ifdef UNKNOWN_ARCH

namespace smbm {
extern load_func_t get_architecture_load_func(GlobalState *g) {
    return simple_long_sum_test;
}

}
#endif