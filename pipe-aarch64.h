#include "features.h"

namespace smbm {
namespace {

static inline void gen_iadd(char *&p, int operand0, int operand1) {
    *(uint32_t*)p = 0x8b000000 | operand0 | (operand1<<5) | (operand1<<16);
    p += 4;
}
static inline void gen_imul(char *&p, int operand0, int operand1) {
    *(uint32_t*)p = 0x9b007c00 | operand0 | (operand1<<5) | (operand1<<16);
    p += 4;
}


static inline void gen_ior(char *&p, int operand0, int operand1) {
    *(uint32_t*)p = 0xaa000000 | operand0 | (operand1<<5) | (operand1<<16);
    p += 4;
}

static inline void gen_iclear(char *&p, int operand0) {
    *(uint32_t*)p = 0xd2800000 | operand0;
    p += 4;
}

static inline void gen_fadd(char *&p, int operand0, int operand1) {
    *(uint32_t*)p = 0x1e602800 | operand0 | (operand1<<5) | (operand1<<16);
    p += 4;
}

static inline void gen_load64(char *&p, int operand0, int operand1) {
    *(uint32_t*)p = 0xf9400000 | operand0 | (operand1<<5);
    p += 4;
}

static inline void gen_nop(char *&p) {
    *(uint32_t*)p = 0xd503201f;
    p += 4;
}

static inline void gen_setimm64(char *&p, int operand, uint64_t val) {
    /* mov operand, val */
    *(uint32_t*)p = 0xd2800000 | ((val&0xffff)<<5) | operand;
    p+=4;
    val >>= 16;

    /* movk operand, val, lsl #16 */
    *(uint32_t*)p = 0xf2800000 | ((val&0xffff)<<5) | operand | (1<<21);
    p+=4;
    val >>= 16;

    /* movk operand, val, lsl #32 */
    *(uint32_t*)p = 0xf2800000 | ((val&0xffff)<<5) | operand | (2<<21);
    p+=4;
    val >>= 16;

    /* movk operand, val, lsl #48 */
    *(uint32_t*)p = 0xf2800000 | ((val&0xffff)<<5) | operand | (3<<21);
    p+=4;
}

template <typename TI, typename TF>
static void enter_frame(char *&p, TI callee_save_int, TF callee_save_fp) {
}

template <typename TI, typename TF>
void exit_frame(char *&p, TI callee_save_int, TF callee_save_fp) {
    /* ret */
    *(uint32_t*)p = 0xd65f03c0;
    p += 4;
}

static std::vector<int> const simple_regs = {
    0, 1, 2, 3,
    4, 5, 6, 7};

static std::initializer_list<int> constexpr callee_saved_int_regs = {};
static std::initializer_list<int> constexpr callee_saved_fp_regs = {};

static void loop_start(char *&p, int loop_count) {
    /* mov x0, loop_count */
    gen_setimm64(p, 0, loop_count);
}

static void loop_end(char *&p, char *loop_start_pos) {
    /* sub x0, x0, 1 */
    *(uint32_t*)p = 0xd1000000 | (1<<10) | (0<<5) | 0;
    p += 4;

    /* cbnz x0, loop_start */
    int delta = ((loop_start_pos-p)>>2)&((1<<19)-1);
    *(uint32_t*)p = 0xb5000000 | (delta<<5) | 0;
    p += 4;
}

} // namespace
} // namespace smbm
