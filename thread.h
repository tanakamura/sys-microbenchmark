#pragma once 
namespace smbm {

typedef pthread_t thread_handle_t;

#ifdef WINDOWS
#elif defined HAVE_GNU_CPU_SET
inline thread_handle_t spawn_thread_on_proc(void *(*start_routine)(void*), void *arg, int cpu, ProcessorTable const *proc_table) {
    int ncpu = proc_table->ncpu_configured;

    cpu_set_t *sptr = CPU_ALLOC(ncpu);
    CPU_ZERO_S(proc_table->ss(), sptr);
    CPU_SET_S(cpu, proc_table->ss(), sptr);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setaffinity_np(&attr, proc_table->ss(), sptr);

    pthread_t ret;
    pthread_create(&ret, &attr, start_routine, arg);
    pthread_attr_destroy(&attr);
    CPU_FREE(sptr);

    return ret;
}

inline void wait_thread(thread_handle_t t) {
    pthread_join(t, NULL);
}
#else
#error "spawn_thread_on_proc"
#endif

}