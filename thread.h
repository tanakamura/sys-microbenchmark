#pragma once 
namespace smbm {

typedef pthread_t thread_handle_t;

inline thread_handle_t spawn_thread_on_proc(void *(*start_routine)(void*), void *arg, int cpu, CPUSet const *full_set) {
    int ncpu = full_set->ncpu_all;
    cpu_set_t *sptr = CPU_ALLOC(ncpu);
    CPU_ZERO_S(full_set->ss(), sptr);
    CPU_SET_S(cpu, full_set->ss(), sptr);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setaffinity_np(&attr, full_set->ss(), sptr);

    pthread_t ret;
    pthread_create(&ret, &attr, start_routine, arg);
    pthread_attr_destroy(&attr);
    CPU_FREE(sptr);

    return ret;
}

inline void wait_thread(thread_handle_t t) {
    pthread_join(t, NULL);
}

}