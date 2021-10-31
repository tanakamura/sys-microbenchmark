#ifdef JS_INTERFACE
#include <emscripten/bind.h>

#include "sys-microbenchmark.h"

using namespace smbm;

using namespace emscripten;

EMSCRIPTEN_BINDINGS(sys_microbenchmark) {
  class_<GlobalState>("GlobalState")
      ;
}
#endif
