#pragma once

#include "sys-microbenchmark.h"

namespace smbm {

#ifdef X86
bool have_avx();
bool have_avx2();
bool have_avx512f();
#endif

}