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

    __m256 sum0 = _mm256_setzero_ps();
    __m256 sum1 = _mm256_setzero_ps();
    __m256 sum2 = _mm256_setzero_ps();
    __m256 sum3 = _mm256_setzero_ps();

    const __m256 *vsrc = (const __m256*)src;

    for (size_t i = 0; i < nloop; i += 4) {
        sum0 = _mm256_or_ps(sum0, vsrc[i + 0]);
        sum1 = _mm256_or_ps(sum1, vsrc[i + 1]);
        sum2 = _mm256_or_ps(sum2, vsrc[i + 2]);
        sum3 = _mm256_or_ps(sum3, vsrc[i + 3]);
    }

    __m256 sum01 = _mm256_or_ps(sum0, sum1);
    __m256 sum23 = _mm256_or_ps(sum2, sum3);

    __m256 sum = _mm256_or_ps(sum01, sum23);

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

void avx256_stream_store(void *dst, size_t sz) {
    size_t nloop = sz / 32;
    __m256i *vdst = (__m256i*)dst;

    for (size_t i = 0; i < nloop; i += 4) {
        _mm256_stream_si256(&vdst[i + 0], _mm256_setzero_si256());
        _mm256_stream_si256(&vdst[i + 1], _mm256_setzero_si256());
        _mm256_stream_si256(&vdst[i + 2], _mm256_setzero_si256());
        _mm256_stream_si256(&vdst[i + 3], _mm256_setzero_si256());
    }
}


}

#endif