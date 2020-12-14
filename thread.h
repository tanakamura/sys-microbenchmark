#pragma once 

#include "cpuset.h"

namespace smbm {

typedef pthread_t thread_handle_t;

#ifdef WINDOWS

#elif defined POSIX
inline thread_handle_t spawn_thread_on_proc(void *(*start_routine)(void*), void *arg) {
    pthread_t t;
    pthread_create(&t, nullptr, start_routine,arg);
    return t;
}

inline void wait_thread(thread_handle_t t) {
    pthread_join(t, NULL);
}
#else
#error "spawn_thread_on_proc"
#endif

}