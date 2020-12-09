#include "x86funcs.h"

#ifdef X86

#include <immintrin.h>

namespace smbm {
void avx512_copy(void *dst, void const *src, size_t sz) {}
uint64_t avx512_load(void const *src, size_t sz) {
    return 0;
}
void avx512_store(void *dst, size_t sz) {
}
}

#endif