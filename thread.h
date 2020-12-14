#pragma once 

#include "cpuset.h"

#ifdef WINDOWS
#include <process.h>
#endif

namespace smbm {

#ifdef WINDOWS

typedef HANDLE thread_handle_t;

inline thread_handle_t spawn_thread(void *(*start_routine)(void*), void *arg) {
    unsigned int threadID;
    HANDLE ret = (HANDLE)_beginthreadex(NULL, 0, (unsigned __stdcall (*)(void*))start_routine, arg, 0, &threadID);
    return ret;
}

inline void wait_thread(thread_handle_t t) {
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
}

#elif defined POSIX
typedef pthread_t thread_handle_t;

inline thread_handle_t spawn_thread(void *(*start_routine)(void*), void *arg) {
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