#include "x86funcs.h"
#include "actual-freq.h"

#ifdef X86

#include <immintrin.h>

namespace smbm {

void avx512_copy(void *dst, void const *src, size_t sz) {
    size_t nloop = sz / 64;
    __m512 *vdst = (__m512*)dst;
    const __m512 *vsrc = (__m512 const *)src;

    for (size_t i = 0; i < nloop; i += 4) {
        vdst[i + 0] = vsrc[i + 0];
        vdst[i + 1] = vsrc[i + 1];
        vdst[i + 2] = vsrc[i + 2];
        vdst[i + 3] = vsrc[i + 3];
    }
}

uint64_t avx512_load(void const *src, size_t sz) {
    size_t nloop = sz / 64;
    //__m128 *vdst = dst;

    __m512i sum0 = _mm512_setzero_si512();
    __m512i sum1 = _mm512_setzero_si512();
    __m512i sum2 = _mm512_setzero_si512();
    __m512i sum3 = _mm512_setzero_si512();

    const __m512i *vsrc = (const __m512i*)src;

    for (size_t i = 0; i < nloop; i += 4) {
        sum0 = _mm512_or_epi64(sum0, vsrc[i + 0]);
        sum1 = _mm512_or_epi64(sum1, vsrc[i + 1]);
        sum2 = _mm512_or_epi64(sum2, vsrc[i + 2]);
        sum3 = _mm512_or_epi64(sum3, vsrc[i + 3]);
    }

    __m512i sum01 = _mm512_or_epi64(sum0, sum1);
    __m512i sum23 = _mm512_or_epi64(sum2, sum3);

    __m512i isum = _mm512_or_epi64(sum01, sum23);

    __m256i sum256i = _mm512_extracti64x4_epi64(isum,0) | _mm512_extracti64x4_epi64(isum,1);
    __m128i sum128i = _mm256_extractf128_si256(sum256i,0) | _mm256_extractf128_si256(sum256i,1);

    return _mm_extract_epi64(sum128i,0) | _mm_extract_epi64(sum128i,1);
}
void avx512_store(void *dst, size_t sz) {
    size_t nloop = sz / 64;
    __m512 *vdst = (__m512*)dst;
    // const volatile __m128 *vsrc = src;

    for (size_t i = 0; i < nloop; i += 4) {
        vdst[i + 0] = _mm512_setzero_ps();
        vdst[i + 1] = _mm512_setzero_ps();
        vdst[i + 2] = _mm512_setzero_ps();
        vdst[i + 3] = _mm512_setzero_ps();
    }
}

void avx512_stream_store(void *dst, size_t sz) {
    size_t nloop = sz / 64;
    __m512i *vdst = (__m512i*)dst;

    for (size_t i = 0; i < nloop; i += 4) {
        _mm512_stream_si512(&vdst[i + 0], _mm512_setzero_si512());
        _mm512_stream_si512(&vdst[i + 1], _mm512_setzero_si512());
        _mm512_stream_si512(&vdst[i + 2], _mm512_setzero_si512());
        _mm512_stream_si512(&vdst[i + 3], _mm512_setzero_si512());
    }
}


using namespace af;
uint64_t busy_iadd32x16(uint64_t zero, oneshot_timer *ot) {
    typedef uint32_t vec512i __attribute__((vector_size(64)));

    vec512i x = {(uint32_t)zero};
    vec512i ret = op(x, x, x, ot, [](auto x, auto y) { return x+y;});
    return ret[0] + ret[1] + ret[2] + ret[3] + ret[4] + ret[5] + ret[6] + ret[7];
}

using namespace af;
uint64_t busy_imul32x16(uint64_t zero, oneshot_timer *ot) {
    typedef uint32_t vec512i __attribute__((vector_size(64)));

    vec512i x = {(uint32_t)zero};
    vec512i ret = op(x, x, x, ot, [](auto x, auto y) { return x*y;});
    return ret[0] + ret[1] + ret[2] + ret[3] + ret[4] + ret[5] + ret[6] + ret[7];
}

using namespace af;
uint64_t busy_fadd64x8(uint64_t zero, oneshot_timer *ot) {
    typedef double vec512d __attribute__((vector_size(64)));

    vec512d x = {(double)zero};
    vec512d y = x + x;
    vec512d ret = opf(x, y, x, ot, [](auto x, auto y) { return x+y;});
    return ret[0] + ret[1] + ret[2] + ret[3];
}

using namespace af;
uint64_t busy_fmul64x8(uint64_t zero, oneshot_timer *ot) {
    typedef double vec512d __attribute__((vector_size(64)));

    vec512d x = {(double)zero};
    vec512d y = x + x;
    vec512d ret = opf(x, y, x, ot, [](auto x, auto y) { return x*y;});
    return ret[0] + ret[1] + ret[2] + ret[3];
}

using namespace af;
uint64_t busy_fma64x8(uint64_t zero, oneshot_timer *ot) {
    typedef double vec512d __attribute__((vector_size(64)));

    vec512d x = {(double)1.93};
    vec512d y = x + x;
    vec512d ret = opf(x, y, x, ot, [](auto x, auto y) { return x*y+x;});
    return ret[0] + ret[1] + ret[2] + ret[3];
}

}

#endif