#include "oneshot_timer.h"
#include "simple-run.h"
#include "sys-microbenchmark.h"
#include "table.h"

#if (defined POSIX) || (defined WASI)
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef POSIX
#include <pthread.h>
#include <sys/mman.h>
#include <sys/wait.h>
#endif

#ifdef WINDOWS
#include <process.h>
#endif

namespace smbm {


namespace {

#ifndef BAREMETAL
struct no_arg {
    void *alloc_arg() { return nullptr; };
    void free_arg(void *p) {}
};

// struct nop : public no_arg {
//    void run(void *arg) {  }
//};

struct close_invalid : public no_arg {
#ifdef WINDOWS
    void run(void *arg) { CloseHandle(INVALID_HANDLE_VALUE); }
#else
    void run(void *arg) { close(-1); }
#endif
};

struct open_close : public no_arg {
    void run(void *arg) {
#ifdef WINDOWS
        HANDLE h =
            CreateFileW(L"C:\\Windows\\system.ini", GENERIC_READ, 0, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            puts("CreateFile(system.ini)");
            exit(1);
        }
        CloseHandle(h);
#else
        int fd = open(".", O_RDONLY);
        if (fd < 0) {
            perror("open .");
            exit(1);
        }
        close(fd);
#endif
    }
};

#ifdef WINDOWS
struct pipe_close : public no_arg {
    void run(void *arg) {
        HANDLE read_pipe;
        HANDLE write_pipe;
        CreatePipe(&read_pipe, &write_pipe, NULL, 0);
        CloseHandle(read_pipe);
        CloseHandle(write_pipe);
    }
};
#elif defined POSIX
struct pipe_close : public no_arg {
    void run(void *arg) {
        int fd[2];
        pipe(fd);
        close(fd[0]);
        close(fd[1]);
    }
};
#endif

#ifdef POSIX
struct select_0 : public no_arg {
    void run(void *arg) {
        fd_set rd, wr, except;
        FD_ZERO(&rd);
        FD_ZERO(&wr);
        FD_ZERO(&except);

        FD_SET(0, &rd);
        FD_SET(0, &wr);
        FD_SET(0, &except);

        struct timeval tv = {0};

        int r = select(1, &rd, &wr, &except, &tv);
        if (r < 0) {
            perror("select");
            exit(1);
        }
    }
};

struct fork_wait : public no_arg {
    void run(void *arg) __attribute__((noinline)) {
        pid_t c;

        if ((c = fork()) < 0) {
            perror("fork");
            exit(1);
        }
        if (c == 0) {
            _exit(1);
        }

        int st;
        waitpid(c, &st, 0);
    }
};
#endif

#ifdef WINDOWS
struct mmap_unmap : public no_arg {
    HANDLE alloc_arg() {
        HANDLE ret = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL,
                                        PAGE_EXECUTE_READWRITE, 0, 4096, NULL);
        if (ret == INVALID_HANDLE_VALUE) {
            puts("CreateFileMappingW");
            exit(1);
        }
        return ret;
    }

    void run(HANDLE h) {
        void *p = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 4096);
        if (p == NULL) {
            puts("MapViewOfFile");
            exit(1);
        }
        UnmapViewOfFile(p);
    }
};
#elif defined POSIX
struct mmap_unmap : public no_arg {
    void run(void *arg) {

        void *p = mmap(0, 4096, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
        if (p == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }
        munmap(p, 4096);
    }
};
#endif

#ifdef WINDOWS
struct write_devnull_1byte {
    HANDLE alloc_arg() {
        HANDLE h = CreateFileW(L"nul", GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            puts("CreateFileW(nul)");
            exit(1);
        }

        return h;
    }

    void free_arg(HANDLE h) { CloseHandle(h); }

    void run(HANDLE h) {
        char x = 0;
        DWORD wrsz;
        BOOL r = WriteFile(h, &x, 1, &wrsz, NULL);
        if (!r) {
            puts("WriteFile");
            exit(1);
        }
    }
};
#elif defined POSIX
struct write_devnull_1byte {
    int alloc_arg() {
        int ret;
        ret = open("/dev/null", O_RDWR);
        if (ret == -1) {
            perror("/dev/null");
            exit(1);
        }

        return ret;
    }

    void free_arg(int fd) { close(fd); }

    void run(int fd) {
        char x = 0;
        ssize_t wrsz = write(fd, &x, 1);
        if (wrsz != 1) {
            perror("write");
            exit(1);
        }
    }
};
#endif

static void *thread_func(void *) { return nullptr; }

struct thread_create_join : public no_arg {
    void run(void *arg) {
#ifdef WINDOWS
        unsigned int threadID;
        HANDLE ret = (HANDLE)_beginthreadex(
            NULL, 0, (unsigned __stdcall (*)(void *))thread_func, arg, 0,
            &threadID);
        WaitForSingleObject(ret, INFINITE);
        CloseHandle(ret);
#elif defined POSIX
        pthread_t t;
        int r = pthread_create(&t, NULL, thread_func, nullptr);
        if (r != 0) {
            perror("pthread_create");
            exit(1);
        }

        pthread_join(t, NULL);
#endif
    }
};

struct gettimeofday1 : public no_arg {
    void run(void *arg) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
    }
};

#ifdef POSIX
struct clock_gettime1 : public no_arg {
    void run(void *arg) {
        struct timespec tv;
        clock_gettime(CLOCK_MONOTONIC, &tv);
    }
};
struct stat1 : public no_arg {
    void run(void *arg) {
        struct stat st;
        stat(".", &st);
    }
};
struct fstat1 : public no_arg {
    int alloc_arg() {
        int ret = open(".", O_RDONLY);
        if (ret == -1) {
            perror("open");
        }
        return ret;
    }
    void free_arg(int fd) { close(fd); }

    void run(int fd) {
        struct stat st;
        fstat(fd, &st);
    }
};

#endif

#ifdef WINDOWS
struct QueryPerformanceCounter1 : public no_arg {
    void run(void *p) {
        LARGE_INTEGER v;
        QueryPerformanceCounter(&v);
    }
};

struct create_destroy_window {
    HINSTANCE alloc_arg() { return (HINSTANCE)GetModuleHandle(NULL); }

    void free_arg(HINSTANCE h) {}

    void run(HINSTANCE hInstance) {
        HWND w =
            CreateWindowW(L"BUTTON", L"test", WS_OVERLAPPED | BS_DEFPUSHBUTTON,
                          0, 0, 10, 10, NULL, NULL, hInstance, NULL);
        if (w == NULL) {
            int er = (int)GetLastError();
            printf("CreateWindow 0x%08x %d\n", er, er);
            exit(1);
        }
        DestroyWindow(w);
    }
};

struct PeekMessage1 : public no_arg {
    void run(void *p) {
        MSG m;
        PeekMessage(&m, NULL, 0, 0, PM_NOREMOVE);
    }
};

struct GetFileAttributes1 : public no_arg {
    void run(void *p) { GetFileAttributesW(L"."); }
};
struct GetFileSize1 : public no_arg {
    HANDLE alloc_arg() {
        HANDLE h = CreateFileW(L"nul", GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            puts("CreateFileW(nul)");
            exit(1);
        }

        return h;
    }

    void free_arg(HANDLE h) { CloseHandle(h); }

    void run(HANDLE h) {
        DWORD len;
        GetFileSize(h, &len);
    }
};

#endif


#define FOR_EACH_TEST_GENERIC(F)                                               \
    /*F(nop)*/                                                                 \
    F(close_invalid)                                                           \
    F(open_close)

#define FOR_EACH_TEST_POSIX(F)                                                 \
    F(select_0)                                                                \
    F(fork_wait)                                                               \
    F(gettimeofday1)                                                           \
    F(clock_gettime1)                                                          \
    F(stat1)                                                                   \
    F(fstat1)                                                                  \
    F(pipe_close)                                                              \
    F(mmap_unmap)                                                              \
    F(thread_create_join)                                                      \
    F(write_devnull_1byte)

#define FOR_EACH_TEST_WINDOWS(F)                                               \
    F(QueryPerformanceCounter1)                                                \
    F(create_destroy_window)                                                   \
    F(PeekMessage1)                                                            \
    F(GetFileAttributes1)                                                      \
    F(GetFileSize1)                                                            \
    F(pipe_close)                                                              \
    F(mmap_unmap)                                                              \
    F(thread_create_join)                                                      \
    F(write_devnull_1byte)

#ifdef POSIX
#define FOR_EACH_TEST(F)                                                       \
    FOR_EACH_TEST_GENERIC(F)                                                   \
    FOR_EACH_TEST_POSIX(F)

#elif defined WINDOWS

#define FOR_EACH_TEST(F)                                                       \
    FOR_EACH_TEST_GENERIC(F)                                                   \
    FOR_EACH_TEST_WINDOWS(F)

#else
#define FOR_EACH_TEST(F) FOR_EACH_TEST_GENERIC(F)

#endif

#endif

struct Syscall : public BenchDesc {
    Syscall() : BenchDesc("syscall") {}

#ifdef BAREMETAL
    virtual result_t run(GlobalState const *g) override {
        return result_t();
    }
    virtual bool available(GlobalState const *g) override { return false; }
#else
    virtual bool available(GlobalState const *g) override { return true; }

    virtual result_t run(GlobalState const *g) override {
        int count = 0;
#define INC_COUNT(F) count++;
        FOR_EACH_TEST(INC_COUNT);

        typedef Table1D<double, std::string> result_t;
        result_t *result = new result_t("test_name", count);

#define NAME(F) #F,

        result->column_label = " nsec/call";
        result->row_label = std::vector<std::string>{FOR_EACH_TEST(NAME)};

        count = 0;

#define RUN(F)                                                                 \
    {                                                                          \
        F t;                                                                   \
        (*result)[count++] = run_test(g, &t);                                  \
    }
        FOR_EACH_TEST(RUN)

        return std::unique_ptr<BenchResult>(result);
    }

#endif

    virtual result_t parse_json_result(picojson::value const &v) override {
        return result_t(Table1D<double, std::string>::parse_json_result(v));
    }
};

} // namespace


std::unique_ptr<BenchDesc> get_syscall_desc() {
    return std::unique_ptr<BenchDesc>(new Syscall());
}

} // namespace smbm
