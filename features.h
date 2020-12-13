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

#define HAVE_HW_CPUCYCLE
#define HAVE_CLOCK_GETTIME
#define HAVE_GNU_CPU_SET
#include <time.h>
#endif

#if (defined X86) || (defined __aarch64__)
#define HAVE_USERLAND_CPUCOUNTER
#endif

#ifndef HAVE_USERLAND_CPUCOUNTER
#define USE_OS_TIMECOUNTER
#endif

namespace smbm {
typedef uint64_t vec128i __attribute__((vector_size(16)));
}
