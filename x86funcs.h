#pragma once

#include "features.h"
#include <stddef.h>
#include <stdint.h>

namespace smbm {

#ifdef X86
extern void sse_stream_store(void *dst, size_t sz);
extern void sse_stream_copy(void *dst, const void *src, size_t sz);

extern void avx256_copy(void *dst, void const *src, size_t sz);
extern uint64_t avx256_load(void const *src, size_t sz);
extern void avx256_store(void *dst, size_t sz);
extern void avx256_stream_store(void *dst, size_t sz);

extern void avx512_copy(void *dst, void const *src, size_t sz);
extern uint64_t avx512_load(void const *src, size_t sz);
extern void avx512_store(void *dst, size_t sz);
extern void avx512_stream_store(void *dst, size_t sz);

extern void x86_rep_movs1(void *dst, void const *src, size_t sz);
extern void x86_rep_movs2(void *dst, void const *src, size_t sz);
extern void x86_rep_movs4(void *dst, void const *src, size_t sz);

extern uint64_t x86_rep_scas1(void const *src, size_t sz);
extern uint64_t x86_rep_scas2(void const *src, size_t sz);
extern uint64_t x86_rep_scas4(void const *src, size_t sz);

extern void x86_rep_stos1(void *dst, size_t sz);
extern void x86_rep_stos2(void *dst, size_t sz);
extern void x86_rep_stos4(void *dst, size_t sz);

#endif


}