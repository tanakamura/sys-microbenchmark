#include "x86funcs.h"

#ifdef X86

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

    __asm__ __volatile__("rep scasb"
                         :"+c"(ecx)
                         :"D"(src)
                         :"memory"
        );

    return ecx;
}
uint64_t x86_rep_scas2(void const *src, size_t sz) {
    uint32_t ecx = sz/2;

    __asm__ __volatile__("rep scasw"
                         :"+c"(ecx)
                         :"D"(src)
                         :"memory"
        );

    return ecx;
}
uint64_t x86_rep_scas4(void const *src, size_t sz) {
    uint32_t ecx = sz/4;

    __asm__ __volatile__("rep scasl"
                         :"+c"(ecx)
                         :"D"(src)
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


}

#endif