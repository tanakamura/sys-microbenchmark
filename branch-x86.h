Loop(int ninsn, bool indirect_branch, int nindirect_brach_target) {
    uint32_t table_size = 0;
    size_t alloc_size = 0;
    size_t inst_size = 0;
    uint64_t *jmp_table = 0;

    if (indirect_branch) {
        inst_size = ninsn * ((12 + 3) + nindirect_brach_target) + 11 + 7;
        table_size = ninsn * nindirect_brach_target * 8;
        alloc_size = inst_size + table_size;
    } else {
        inst_size = ninsn * (7 + 3) + 11;
        alloc_size = inst_size;
    }

    this->em = alloc_exeutable(alloc_size);
    char *p = (char *)this->em.p;
    char *p0 = p;

    this->inst_len = inst_size;

    if (indirect_branch) {
        uint32_t table_delta = inst_size;

        /* lea r9, [rip + table_delta] */
        *(p++) = 0x4c; // 1
        *(p++) = 0x8d; // 2
        *(p++) = 0x0d; // 3

        output4(p, table_delta - 7); // 7
        jmp_table = (uint64_t *)(p0 + inst_size);
    }

#ifdef WINDOWS
    uint8_t first_argument = 0x1;  /* rcx */
    uint8_t second_argument = 0x2; /* rdx */
#else
    uint8_t first_argument = 0x7;  /* rdi */
    uint8_t second_argument = 0x6; /* rsi */
#endif

    *(p++) = 0x31; // 1
    *(p++) = 0xc0; /* 2 : xor eax, eax */

    char *loop_start = p;

    for (int i = 0; i < ninsn; i++) {
        /* if (*first_argument) ret++ */

        if (indirect_branch) {
            /* movzq r8, byte [first_argument] */
            *(p++) = 0x4c;           // 1
            *(p++) = 0x0f;           // 2
            *(p++) = 0xb6;           // 3
            *(p++) = first_argument; // 4

            uint32_t table_offset = i * nindirect_brach_target * 8;

            /* jmp [r9 + r8 *8 + table_offset] */
            *(p++) = 0x43;            // 5
            *(p++) = 0xff;            // 6
            *(p++) = 0xa4;            // 7
            *(p++) = 0xc1;            // 8
            output4(p, table_offset); // 12

            for (int ti = 0; ti < nindirect_brach_target; ti++) {
                jmp_table[i * nindirect_brach_target + ti] = (uintptr_t)p;
                *(p++) = 0x90; // nop
            }
        } else {
            /* cmp [first_argument], 0 */
            *(p++) = 0x80;                    // 1
            *(p++) = 0x38 | (first_argument); // 2
            *(p++) = 1;                       // 3

            /* jne +2 */
            *(p++) = 0x75; // 4
            *(p++) = 0x2;  // 5

            /* inc eax */
            *(p++) = 0xff; // 6
            *(p++) = 0xc0; // 7
        }

        /* inc first_argument */
        *(p++) = 0x48;                  // 1
        *(p++) = 0xff;                  // 2
        *(p++) = 0xc0 | first_argument; // 3
    }

    /* dec second_argument */
    *(p++) = 0xff;                   // 3
    *(p++) = 0xc8 | second_argument; // 4

    /* jnz loop_begin */
    uint64_t delta = (p - loop_start) + 6;
    uint32_t disp = -delta;

    *(p++) = 0x0f;    // 5
    *(p++) = 0x85;    // 6
    output4(p, disp); // 10

    *(p++) = 0xc3; // 11

    size_t actual_length = p - p0;
    if (actual_length != inst_size) {
        fprintf(stderr, "actual:%d alloc:%d\n", (int)actual_length,
                (int)inst_size);
        // abort();
    }
}
