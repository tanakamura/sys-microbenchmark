#include "x86funcs.h"

#ifdef X86
#include <immintrin.h>
#include <stdint.h>

namespace smbm {
void avx256_copy(void *dst, void const *src, size_t sz) {
    size_t nloop = sz / 32;
    __m256 *vdst = (__m256*)dst;
    const __m256 *vsrc = (__m256 const *)src;

    for (size_t i = 0; i < nloop; i += 4) {
        vdst[i + 0] = vsrc[i + 0];
        vdst[i + 1] = vsrc[i + 1];
        vdst[i + 2] = vsrc[i + 2];
        vdst[i + 3] = vsrc[i + 3];
    }
}

uint64_t avx256_load(void const *src, size_t sz) {
    size_t nloop = sz / 32;
    //__m128 *vdst = dst;

    __m256 sum = _mm256_setzero_ps();
    const __m256 *vsrc = (const __m256*)src;

    for (size_t i = 0; i < nloop; i += 4) {
        sum = _mm256_or_ps(sum, vsrc[i + 0]);
        sum = _mm256_or_ps(sum, vsrc[i + 1]);
        sum = _mm256_or_ps(sum, vsrc[i + 2]);
        sum = _mm256_or_ps(sum, vsrc[i + 3]);
    }

    __m256i isum = _mm256_castps_si256(sum);
    __m128i sum128i = _mm256_extractf128_si256(isum,0) | _mm256_extractf128_si256(isum,1);

    return _mm_extract_epi64(sum128i,0) | _mm_extract_epi64(sum128i,1);
}
void avx256_store(void *dst, size_t sz) {
    size_t nloop = sz / 32;
    __m256 *vdst = (__m256*)dst;
    // const volatile __m128 *vsrc = src;

    for (size_t i = 0; i < nloop; i += 4) {
        vdst[i + 0] = _mm256_setzero_ps();
        vdst[i + 1] = _mm256_setzero_ps();
        vdst[i + 2] = _mm256_setzero_ps();
        vdst[i + 3] = _mm256_setzero_ps();
    }

}
}

#endif