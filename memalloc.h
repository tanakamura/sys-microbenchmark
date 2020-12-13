#pragma once

#include <stdlib.h>
#include <string.h>

namespace smbm {

inline void *aligned_calloc(size_t align, size_t size) {
    void *p = 0;
    posix_memalign(&p, align, size);
    memset(p, 0, size);
    return p;
}

inline void aligned_free(void *p) {
    free(p);
}

}