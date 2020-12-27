struct no_arg {
    void *alloc_arg() { return nullptr; }
    void free_arg(void *) {}
};

struct dsb_sy : public no_arg {
    void run(void *) { __asm__ __volatile__("dsb sy" ::: "memory"); }
};
struct dsb_ld : public no_arg {
    void run(void *) { __asm__ __volatile__("dsb ld" ::: "memory"); }
};
struct dsb_st : public no_arg {
    void run(void *) { __asm__ __volatile__("dsb st" ::: "memory"); }
};

struct dmb_sy : public no_arg {
    void run(void *) { __asm__ __volatile__("dmb sy" ::: "memory"); }
};
struct dmb_ld : public no_arg {
    void run(void *) { __asm__ __volatile__("dmb ld" ::: "memory"); }
};
struct dmb_st : public no_arg {
    void run(void *) { __asm__ __volatile__("dmb st" ::: "memory"); }
};

struct isb : public no_arg {
    void run(void *) { __asm__ __volatile__("isb" ::: "memory"); }
};

struct read_cntvct : public no_arg {
    void run(void *) { uint64_t v64; __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(v64)); }
};

struct nop : public no_arg {
    void run(void *p) { __asm__ __volatile__("nop"); }
};

static bool T() { return true; }

#define FOR_EACH_TEST(F)                                                       \
    F(nop, T)                                                                  \
    F(dsb_sy, T)                                                        \
    F(dsb_ld, T)                                                               \
    F(dsb_st, T)                                                    \
    F(dmb_sy, T)                                                               \
    F(dmb_ld, T)                                                                \
    F(dmb_st, T)                                                                \
    F(isb, T)                                                        \
    F(read_cntvct, T)                                                              \



