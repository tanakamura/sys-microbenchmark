#include "sys-features.h"

namespace smbm {
namespace {

static inline void output4(char *&p, uint32_t v) {
    *(p++) = (v >> (8 * 0)) & 0xff;
    *(p++) = (v >> (8 * 1)) & 0xff;
    *(p++) = (v >> (8 * 2)) & 0xff;
    *(p++) = (v >> (8 * 3)) & 0xff;
}

static inline void output8(char *&p, uint64_t v) {
    *(p++) = (v >> (8 * 0)) & 0xff;
    *(p++) = (v >> (8 * 1)) & 0xff;
    *(p++) = (v >> (8 * 2)) & 0xff;
    *(p++) = (v >> (8 * 3)) & 0xff;
    *(p++) = (v >> (8 * 4)) & 0xff;
    *(p++) = (v >> (8 * 5)) & 0xff;
    *(p++) = (v >> (8 * 6)) & 0xff;
    *(p++) = (v >> (8 * 7)) & 0xff;
}

static inline void gen_iadd(char *&p, int operand0, int operand1) {
    /* add operand[0], operand[1] */

    // op0 = reg
    // op1 = modrm

    int op0_r = (operand0 >> 3) << 0;
    int op1_b = (operand1 >> 3) << 2;

    int op0_modrm = (operand0 & 7) << 3;
    int op1_modrm = operand1 & 7;

    *(p++) = 0x48 | op0_r | op1_b;
    *(p++) = 0x3;
    *(p++) = 0xc0 | op0_modrm | op1_modrm;
}

static inline void gen_imul(char *&p, int operand0, int operand1) {
    int op0_r = (operand0 >> 3) << 0;
    int op1_b = (operand1 >> 3) << 2;

    int op0_modrm = (operand0 & 7) << 3;
    int op1_modrm = operand1 & 7;

    *(p++) = 0x48 | op0_r | op1_b;
    *(p++) = 0x0f;
    *(p++) = 0xaf;
    *(p++) = 0xc0 | op0_modrm | op1_modrm;
}

static inline void gen_ior(char *&p, int operand0, int operand1) {
    int op0_r = (operand0 >> 3) << 0;
    int op1_b = (operand1 >> 3) << 2;

    int op0_modrm = (operand0 & 7) << 3;
    int op1_modrm = operand1 & 7;

    *(p++) = 0x48 | op0_r | op1_b;
    *(p++) = 0x0b;
    *(p++) = 0xc0 | op0_modrm | op1_modrm;
}

static inline void gen_iclear(char *&p, int operand0) {
    int operand1 = operand0;

    int op0_r = (operand0 >> 3) << 0;
    int op1_b = (operand1 >> 3) << 2;

    int op0_modrm = (operand0 & 7) << 3;
    int op1_modrm = operand1 & 7;

    *(p++) = 0x48 | op0_r | op1_b;
    *(p++) = 0x31;
    *(p++) = 0xc0 | op0_modrm | op1_modrm;
}

static inline void gen_imm0(char *&p, int operand0) {
    int operand1 = operand0;

    int op0_r = (operand0 >> 3) << 0;
    int op1_b = (operand1 >> 3) << 2;

    int op0_modrm = (operand0 & 7) << 3;
    int op1_modrm = operand1 & 7;

    *(p++) = 0x48 | op0_r | op1_b;
    *(p++) = 0x31;
    *(p++) = 0xc0 | op0_modrm | op1_modrm;
}

static inline void gen_fadd(char *&p, int operand0, int operand1) {
    int op0_r = (operand0 >> 3) << 0;
    int op1_b = (operand1 >> 3) << 2;
    int op0_modrm = (operand0 & 7) << 3;
    int op1_modrm = (operand1 & 7);

    *(p++) = 0xf3;
    *(p++) = 0x40 | op0_r | op1_b;
    *(p++) = 0x0f;
    *(p++) = 0x58;
    *(p++) = 0xc0 | op0_modrm | op1_modrm;
}

static inline void gen_int_to_fp(char *&p, int operand0, int operand1) {
    /* movq */
    int op0_r = (operand0 >> 3) << 0;
    int op1_b = (operand1 >> 3) << 2;
    int op0_modrm = (operand0 & 7) << 3;
    int op1_modrm = (operand1 & 7);

    *(p++) = 0x66;
    *(p++) = 0x40 | op0_r | op1_b;
    *(p++) = 0x0f;
    *(p++) = 0x6e;
    *(p++) = 0xc0 | op0_modrm | op1_modrm;
}

static inline void gen_load64(char *&p, int operand0, int operand1) {
    int rex_r = (operand1 >> 3) << 0;
    int rex_b = (operand0 >> 3) << 2;

    int op0_modrm = (operand0 & 7) << 3;
    int op1_modrm = (operand1 & 7);

    *(p++) = 0x48 | rex_r | rex_b;
    *(p++) = 0x8b;
    *(p++) = 0x00 | op0_modrm | op1_modrm;
}

static inline void gen_nop(char *&p) { *(p++) = 0x90; }

static inline void gen_setimm64(char *&p, int operand, uint64_t val) {
    int rex_r = (operand >> 3);
    *(p++) = 0x48 | rex_r;
    *(p++) = 0xb8 | (operand & 7);
    output8(p, val);
}

template <typename TI, typename TF>
static void enter_frame(char *&p, TI callee_save_int, TF callee_save_fp) {
    int nint = callee_save_int.size();

    int nfp = callee_save_fp.size();

    int frame_size = nfp * 16 + nint * 8 + 16;

    /* push rbp */
    *(p++) = 0x55;

    /* mov rbp, rsp */
    *(p++) = 0x48;
    *(p++) = 0x89;
    *(p++) = 0xe5;

    *(p++) = 0x48;
    *(p++) = 0x81;
    *(p++) = 0xec;

    output4(p, frame_size);

    int cur = 16;

    for (auto x : callee_save_fp) {
        int rex_r = (x >> 3) << 2;
        int r = x & 7;

        *(p++) = 0x40 | rex_r;
        *(p++) = 0x0f;
        *(p++) = 0x29;
        *(p++) = 0x85 | (r << 3);
        output4(p, -cur);

        cur += 16;
    }

    for (auto x : callee_save_int) {
        int rex_r = (x >> 3) << 2;
        int r = x & 7;

        *(p++) = 0x48 | rex_r;
        *(p++) = 0x89;
        *(p++) = 0x85 | (r << 3);
        output4(p, -cur);

        cur += 8;
    }
}

template <typename TI, typename TF>
void exit_frame(char *&p, TI callee_save_int, TF callee_save_fp) {
    int cur = 16;

    for (auto x : callee_save_fp) {
        int rex_r = (x >> 3) << 2;
        int r = x & 7;

        *(p++) = 0x40 | rex_r;
        *(p++) = 0x0f;
        *(p++) = 0x28;
        *(p++) = 0x85 | (r << 3);
        output4(p, -cur);

        cur += 16;
    }

    for (auto x : callee_save_int) {
        int rex_r = (x >> 3) << 2;
        int r = x & 7;

        *(p++) = 0x48 | rex_r;
        *(p++) = 0x8b;
        *(p++) = 0x85 | (r << 3);
        output4(p, -cur);

        cur += 8;
    }

    /* mov rsp, rbp */
    *(p++) = 0x48;
    *(p++) = 0x89;
    *(p++) = 0xec;

    /* pop rbp */
    *(p++) = 0x5d;
    *(p++) = 0xc3;

    /* ret */
    *(p++) = 0xc3;
}

enum {
    RAX = 0,
    RCX = 1,
    RDX = 2,
    RBX = 3,
    // rsp=4
    // rbp=5
    RSI = 6,
    RDI = 7,

    R8 = 8,
    R9 = 9,
    R10 = 10,
    R11 = 11,
    // R12
    // R13
    R14 = 14,
    R15 = 15,
};

#ifdef WINDOWS
// static auto constexpr caller_saved_int_regs = {RAX, RCX, RDX, R8, R9, R10,
// R11};
static auto constexpr callee_saved_int_regs = {RBX, RDI, RSI, R14, R15};

// static auto constexpr caller_saved_fp_regs = {0,1,2,3,4,5};
static auto constexpr callee_saved_fp_regs = {6,  7,  8,  9,  10,
                                              11, 12, 13, 14, 15};
static std::vector<int> const simple_regs = {0, 1, 2,  3,  6,  7,
                                             8, 9, 10, 11, 14, 15};
#else
// static auto constexpr caller_saved_int_regs = {RAX, RCX, RDX, R8, R9, R10,
// R11, RDI, RSI};

static auto constexpr callee_saved_int_regs = {RBX, R14, R15};

static std::vector<int> const simple_regs = {0, 1, 2,  3,  6,  7,
                                             8, 9, 10, 11, 14, 15};

// static auto constexpr caller_saved_fp_regs = {0, 1, 2,  3,  4,  5,  6,  7,
//                                              8, 9, 10, 11, 12, 13, 14, 15};
static std::initializer_list<int> constexpr callee_saved_fp_regs = {};

#endif

static void loop_start(char *&p, int loop_count) {
    /* mov rax, loop_count */
    *(p++) = 0x48;
    *(p++) = 0xc7;
    *(p++) = 0xc0;
    output4(p, loop_count);
}

static void loop_end(char *&p, char *loop_start_pos) {
    int addr_rel = p - loop_start_pos;
    addr_rel += 3 + 6;

    /* dec %rax */
    *(p++) = 0x48;
    *(p++) = 0xff;
    *(p++) = 0xc8;

    /* jnz addr_rel */
    *(p++) = 0x0f;
    *(p++) = 0x85;
    output4(p, -addr_rel);
}

} // namespace
} // namespace smbm
