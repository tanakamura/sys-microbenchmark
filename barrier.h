#pragma once 

#include "features.h"

#ifdef X86
#include <x86intrin.h>
#endif

namespace smbm {

#ifdef X86
    static inline void rmb() {
        _mm_lfence();
    }
    static inline void wmb() {
        _mm_sfence();
    }
#endif

}