#pragma once

#include <stdlib.h>
#include <string.h>
#include <malloc.h>

namespace smbm {

#ifdef POSIX
inline void *aligned_calloc(size_t align, size_t size) {
    void *p = 0;
    posix_memalign(&p, align, size);
    memset(p, 0, size);
    return p;
}

inline void aligned_free(void *p) {
    free(p);
}
#elif defined WINDOWS
inline void *aligned_calloc(size_t align, size_t size) {
    void *p = _aligned_malloc(size, align);
    memset(p, 0, size);
    return p;
}

inline void aligned_free(void *p) {
    _aligned_free(p);
}
#else
#error "aligned_calloc"
#endif


}