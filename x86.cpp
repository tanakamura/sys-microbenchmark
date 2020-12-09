#include "x86funcs.h"

#ifdef X86
#include <immintrin.h>
namespace smbm {

void x86_rep_movs1(void *dst, void const *src, size_t sz) {
    uint32_t ecx = sz;

    __asm__ __volatile__ ("rep movsb"
                          :"+c"(ecx)
                          :"S"(src),
                           "D"(dst)
                          :"memory"
        );
}
void x86_rep_movs2(void *dst, void const *src, size_t sz) {
    uint32_t ecx = sz/2;

    __asm__ __volatile__ ("rep movsw"
                          :"+c"(ecx)
                          :"S"(src),
                           "D"(dst)
                          :"memory"
        );
}
void x86_rep_movs4(void *dst, void const *src, size_t sz) {
    uint32_t ecx = sz/4;

    __asm__ __volatile__ ("rep movsl"
                          :"+c"(ecx)
                          :"S"(src),
                           "D"(dst)
                          :"memory"
        );
}

uint64_t x86_rep_scas1(void const *src, size_t sz) {
    uint32_t ecx = sz;

    /* assumes : src is cleared to zero */

    __asm__ __volatile__("rep scasb"
                         :"+c"(ecx)
                         :"D"(src),
                          "a"('Z')
                         :"memory"
        );

    return ecx;
}
uint64_t x86_rep_scas2(void const *src, size_t sz) {
    uint32_t ecx = sz/2;

    __asm__ __volatile__("rep scasw"
                         :"+c"(ecx)
                         :"D"(src),
                          "a"('Z')
                         :"memory"
        );

    return ecx;
}
uint64_t x86_rep_scas4(void const *src, size_t sz) {
    uint32_t ecx = sz/4;

    __asm__ __volatile__("rep scasl"
                         :"+c"(ecx)
                         :"D"(src),
                          "a"('Z')
                         :"memory"
        );

    return ecx;
}

void x86_rep_stos1(void *dst, size_t sz) {
    uint32_t ecx = sz;

    __asm__ __volatile__("rep stosb"
                         :"+c"(ecx)
                         :"D"(dst)
                         :"memory"
        );
}
void x86_rep_stos2(void *dst, size_t sz) {
    uint32_t ecx = sz/2;

    __asm__ __volatile__("rep stosw"
                         :"+c"(ecx)
                         :"D"(dst)
                         :"memory"
        );
}
void x86_rep_stos4(void *dst, size_t sz) {
    uint32_t ecx = sz/4;

    __asm__ __volatile__("rep stosl"
                         :"+c"(ecx)
                         :"D"(dst)
                         :"memory"
        );
}

void sse_stream_store(void *dst, size_t sz) {
    size_t nloop = sz / 16;
    __m128i *vd = (__m128i*)dst;
    __m128i zero = _mm_setzero_si128();

    for (size_t i=0; i<nloop; i+=4) {
        _mm_stream_si128(&vd[i + 0], zero);
        _mm_stream_si128(&vd[i + 1], zero);
        _mm_stream_si128(&vd[i + 2], zero);
        _mm_stream_si128(&vd[i + 3], zero);
    }
}

}

#endif
