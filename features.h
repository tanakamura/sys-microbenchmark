#pragma once

#include <stdint.h>

#if (defined __i386__) || (defined __x86_64__)
#define X86
#elif (defined __aarch64__)
#define AARCH64
#else
#define UNKNOWN_ARCH
#endif

#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define HAVE_HWLOC
#define HAVE_HW_PERF_COUNTER
#define HAVE_GNU_CPU_SET

#define POSIX

#include <time.h>

#elif defined _WIN64

#define WINDOWS
#include <windows.h>
#define HAVE_HWLOC
#define HAVE_THREAD

#define clock_gettime w32_clock_gettime_is_not_implemented

#endif

#if (defined X86) || (defined AARCH64)
#define HAVE_USERLAND_CPUCOUNTER
#endif

#ifndef HAVE_USERLAND_CPUCOUNTER
#define USE_OS_TIMECOUNTER
#endif

#ifdef POSIX
#define HAVE_THREAD
#define HAVE_CLOCK_GETTIME
#endif


#ifdef X86
#define yield_thread() _mm_pause()
#define HAVE_YIELD_INSTRCUTION
#define HAVE_DYNAMIC_CODE_GENERATOR
#define HAVE_ARCHITECURE_SPECIFIC_MEMFUNC
#else
#define yield_thread() 
#endif

#ifdef AARCH64
#define HAVE_DYNAMIC_CODE_GENERATOR
#endif

#ifdef __wasi__
#define HAVE_CLOCK_GETTIME
#else

#ifdef EMSCRIPTEN
#define BAREMETAL
#endif

#endif

namespace smbm {
typedef uint64_t vec128i __attribute__((vector_size(16)));
}
