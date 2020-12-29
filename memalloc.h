#pragma once

#include "sys-features.h"

#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#ifdef POSIX
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace smbm {

#if (defined POSIX) || (defined __wasi__) || (defined EMSCRIPTEN)
inline void *aligned_calloc(size_t align, size_t size) {
    void *p = 0;
    posix_memalign(&p, align, size);
    memset(p, 0, size);
    return p;
}

inline void aligned_free(void *p) {
    free(p);
}

#ifdef POSIX
struct ExecutableMemory {
    void *p;
    size_t mapped_size;
};

inline ExecutableMemory alloc_exeutable(size_t sz) {
    size_t pgsz = sysconf(_SC_PAGESIZE);
    size_t map_size = (sz + pgsz-1) & ~(pgsz-1);
    void *p0 = (char *)mmap(0, map_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                            MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

    if (p0 == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    ExecutableMemory ret;
    ret.p = p0;
    ret.mapped_size = map_size;

    return ret;
}

inline void free_executable(ExecutableMemory const *m) {
    munmap(m->p, m->mapped_size);
}
#endif

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
