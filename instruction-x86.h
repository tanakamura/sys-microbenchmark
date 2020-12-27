struct no_arg {
    void *alloc_arg() { return nullptr; }
    void free_arg(void *) {}
};

struct mfence : public no_arg {
    void run(void *) { __asm__ __volatile__("mfence" ::: "memory"); }
};
struct lfence : public no_arg {
    void run(void *) { __asm__ __volatile__("lfence" ::: "memory"); }
};
struct pause : public no_arg {
    void run(void *) { __asm__ __volatile__("pause" ::: "memory"); }
};
struct rdtsc : public no_arg {
    void run(void *) { __asm__ __volatile__("rdtsc" ::: "memory"); }
};
struct rdtscp : public no_arg {
    void run(void *) { __asm__ __volatile__("rdtscp" ::: "memory"); }
};
struct use_1line {
    void *alloc_arg() {
        void *p = aligned_calloc(64, 64);
        return p;
    }
    void free_arg(void *p) { aligned_free(p); }
};

struct sfence : public no_arg {
    void run(void *) { __asm__ __volatile__("sfence" ::: "memory"); }
};
struct sfence_with_store : public use_1line {
    void run(void *p) {
        *(volatile uint32_t *)p = 0;
        __asm__ __volatile__("sfence" ::: "memory");
    }
};

struct clflush : public use_1line{
    void run(void *p) {
        __asm__ __volatile__("clflush 0(%0)" ::"r"(p) : "memory");
    }
};
struct clflushopt : public use_1line{
    void run(void *p) {
        __asm__ __volatile__("clflushopt 0(%0)" ::"r"(p) : "memory");
    }
};
struct clwb : public use_1line {
    void run(void *p) {
        __asm__ __volatile__("clwb 0(%0)" ::"r"(p) : "memory");
    }
};
struct rdrand : public no_arg {
    void run(void *p) {
        uint64_t x;
        __asm__ __volatile__("rdrand %0" : "=r"(x)::"memory");
    }
};
struct vzeroupper : public no_arg {
    void run(void *p) { __asm__ __volatile__("vzeroupper"); }
};
struct vzeroupper_with_avxop : public use_1line {
    void run(void *p) {
        __asm__ __volatile__(
            "vmovd %0, %%xmm0\n\t"
            "vpbroadcastd %%xmm0, %%ymm0\n\t"
            "vzeroupper"
            :
            :"r"(1)
            :"memory", "ymm0"
            );
    }
};
struct nop : public no_arg {
    void run(void *p) { __asm__ __volatile__("nop"); }
};

static bool T() { return true; }

#define FOR_EACH_TEST(F)                                                       \
    F(nop, T)                                                                  \
    F(mfence, T)                                                               \
    F(sfence, T)                                                               \
    F(sfence_with_store, T)                                                    \
    F(lfence, T)                                                               \
    F(pause, T)                                                                \
    F(rdtsc, T)                                                                \
    F(rdtscp, T)                                                               \
    F(clflush, T)                                                              \
    F(clflushopt, have_clflushopt)                                             \
    F(clwb, have_clwb)                                      \
    F(rdrand, have_rdrand)                                                     \
    F(vzeroupper, have_avx)                                             \
    F(vzeroupper_with_avxop, have_avx)
