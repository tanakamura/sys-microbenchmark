#include "cpu-feature.h"

#ifdef X86
#include <cpuid.h>
#endif

namespace smbm {

#ifdef X86

#define CPUID_BIT(cpuid_pos, reg, bit)                                         \
    {                                                                          \
        int EAX, EBX, ECX, EDX;                                                \
        __cpuid(cpuid_pos, EAX, EBX, ECX, EDX);                                \
        return (reg >> bit) & 1;                                               \
    }

#define XCPUID_BIT(cpuid_pos, xpos, reg, bit)                                  \
    {                                                                          \
        int EAX, EBX, ECX, EDX;                                                \
        __cpuid_count(cpuid_pos, xpos, EAX, EBX, ECX, EDX);                    \
        return (reg >> bit) & 1;                                               \
    }

bool have_avx() { CPUID_BIT(1, ECX, 28); }
bool have_avx2() { XCPUID_BIT(7, 0, EBX, 5); }
bool have_fma() { XCPUID_BIT(1, 0, ECX, 12); }
bool have_avx512f() { XCPUID_BIT(7, 0, EBX, 16); }
bool have_clflushopt() { XCPUID_BIT(7, 0, EBX, 23); }
bool have_rdrand() { CPUID_BIT(1, ECX, 30); }
bool have_clwb() { XCPUID_BIT(7,0, EBX, 24); }
bool have_pcommit() { XCPUID_BIT(7,0, EBX, 22); }

#endif
} // namespace smbm
