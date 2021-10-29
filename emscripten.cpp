#ifdef EMSCRIPTEN
#include <emscripten/html5.h>
namespace smbm {
double get_js_tick() {
  return emscripten_performance_now();
}

} // namespace smbm

extern "C" {
void em_entry() {
  return;
}
}

#endif
