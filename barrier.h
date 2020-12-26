#pragma once 

#include "features.h"

#ifdef X86
#include <x86intrin.h>
#endif

namespace smbm {

#define compiler_mb() __asm__ __volatile__ ("":::"memory");


#ifdef X86
    static inline void rmb() {
        _mm_lfence();
    }
    static inline void wmb() {
        _mm_sfence();
    }
#elif defined __aarch64__
    static inline void rmb() {
        __asm__ __volatile__ ("dmb ld" ::: "memory");
    }
    static inline void wmb() {
        __asm__ __volatile__ ("dmb st" ::: "memory");
    }

#elif ! defined HAVE_THREAD
    static inline void rmb() {
        compiler_mb();
    }
    static inline void wmb() {
        compiler_mb();
    }
#endif

}
