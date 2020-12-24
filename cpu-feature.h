#pragma once

#include "features.h"

namespace smbm {

#ifdef X86
bool have_avx();
bool have_avx2();
bool have_fma();
bool have_avx512f();
bool have_clflushopt();
bool have_rdrand();
bool have_clwb();
bool have_pcommit();
#endif

}
