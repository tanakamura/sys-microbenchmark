/* light weight abi */

#include "sys-microbenchmark.h"

using namespace smbm;

extern "C" {

static GlobalState g;
static auto bench_list = g.get_active_benchmark_list();

int smbm_lwabi_get_num_active_benchmark_list() {
  return bench_list.size();
}

const char *smbm_lwabi_get_benchmark_name(int idx) {
  return bench_list[idx]->name.c_str();
}

}
