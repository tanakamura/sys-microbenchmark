//ARM (A64)
//
//The 64-bit ARM (AArch64) calling convention allocates the 31 general-purpose registers as:[2]
//
//    x31 (SP): Stack pointer or a zero register, depending on context.
//    x30 (LR): Procedure link register, used to return from subroutines.
//    x29 (FP): Frame pointer.
//    x19 to x29: Callee-saved.
//    x18 (PR): Platform register. Used for some operating-system-specific special purpose, or an additional caller-saved register.
//    x16 (IP0) and x17 (IP1): Intra-Procedure-call scratch registers.
//    x9 to x15: Local variables, caller saved.
//    x8 (XR): Indirect return value address.
//    x0 to x7: Argument values passed to and results returned from a subroutine.
//
//All registers starting with x have a corresponding 32-bit register prefixed with w. Thus, a 32-bit x0 is called w0.
//
//Similarly, the 32 floating-point registers are allocated as:[3]
//
//    v0 to v7: Argument values passed to and results returned from a subroutine.
//    v8 to v15: callee-saved, but only the bottom 64 bits need to be preserved.
//    v16 to v31: Local variables, caller saved.

Loop(int ninsn, bool indirect_branch, int nindirect_brach_target) {
    uint32_t table_size = 0;
    size_t alloc_size = 0;
    size_t inst_size = 0;
    uint64_t *jmp_table = 0;

    if (indirect_branch) {
        inst_size = ((ninsn * (4 + nindirect_brach_target)) + 6) * 4;
        table_size = ninsn * nindirect_brach_target * 8;
        alloc_size = inst_size + table_size;
    } else {
        inst_size = (ninsn * (3) + 5)*4;
        alloc_size = inst_size;
    }

    this->em = alloc_exeutable(alloc_size);
    uint32_t *p = (uint32_t *)this->em.p;
    char *p0 = (char*)p;

    this->inst_len = inst_size;

    if (indirect_branch) {
        uint32_t table_delta = inst_size;

        /* adr x3, table-delta */
        *(p++) = 0x10000000 | ((table_delta>>2) << 5) | 3;
        jmp_table = (uint64_t *)(p0 + inst_size);
    }

    uint8_t first_argument = 0x0;  /* x0 */
    uint8_t second_argument = 0x1; /* x1 */

    /* mov x4, 0 */
    *(p++) = 0xd2800000 | 4;

    uint32_t *loop_start = p;

    for (int i = 0; i < ninsn; i++) {
        /* if (*first_argument) ret++ */

        if (indirect_branch) {
            /* ldrb w5, [first_argument], 1 */
            *(p++) = 0x38401400 | (first_argument<<5) | 5;

            uint32_t table_offset = i * nindirect_brach_target;

            /* add x5, x5, table_offset */
            *(p++) = 0x91000000 | (table_offset << 10) | (5<<5) | 5;

            /* ldr x5, [x3, x5, lsl 3] */
            *(p++) = 0xf8657865;

            /* br x5 */
            *(p++) = 0xd61f00a0;

            for (int ti=0; ti<nindirect_brach_target; ti++) {
                jmp_table[i*nindirect_brach_target + ti] = (uintptr_t)p;
                /* nop */
                *(p++) = 0xd503201f;
            }
        } else {
            /* ldrb w5, [first_argument], 1 */
            *(p++) = 0x38401400 | (first_argument<<5) | 5;

            /* cbz +2 */
            *(p++) = 0xb4000000 | (2<<5) | 5;

            /* add x4, x4, 1 */
            *(p++) = 0x91000484;
        }
    }

    /* sub second_argument, second_argument, 1 # sub dec second_argument */
    *(p++) = 0xd1000400 | (second_argument<<5) | second_argument;

    /* cbnz loop_begin */
    int delta = (loop_start-p)&((1<<19)-1);
    *(p++) = 0xb5000000 | (delta<<5) | second_argument;

    /* mov x0, x4 */
    *(p++) = 0xaa0403e0;

    /* ret */
    *(p++) = 0xd65f03c0;

    size_t actual_length = ((char*)p) - p0;
    if (actual_length != inst_size) {
        fprintf(stderr, "actual:%d alloc:%d\n", (int)actual_length,
                (int)inst_size);
        abort();
    }

    wmb();
    isync();
}
