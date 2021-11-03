#pragma once 

#include "sys-features.h"

#ifdef X86
#include <x86intrin.h>
#endif

#ifdef HAVE_CLOSE
#include <unistd.h>
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
static inline void isync(void *begin, void *end) {
}
#elif defined AARCH64
static inline void rmb() {
    __asm__ __volatile__ ("dmb ld" ::: "memory");
}
static inline void wmb() {
    __asm__ __volatile__ ("dmb st" ::: "memory");
}
static inline void isync(void *begin, void *end) {
    __builtin___clear_cache((char*)begin, (char*)end);
    __asm__ __volatile__ ("isb" ::: "memory");
}

#elif ! defined HAVE_THREAD
static inline void rmb() {
    compiler_mb();
}
static inline void wmb() {
    compiler_mb();
}
static inline void isync(void *begin, void *end) {
    compiler_mb();
}
#endif

#ifdef HAVE_CLOSE
static inline void
touch_memory() {
    close(-1);
}
#elif defined EMSCRIPTEN
static inline void
touch_memory() {
    EM_ASM("");
}
#else
#error "undefined: touch_memory"
#endif

}
