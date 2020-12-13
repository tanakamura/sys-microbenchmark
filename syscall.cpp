#include "oneshot_timer.h"
#include "sys-microbenchmark.h"
#include "table.h"
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace smbm {

struct no_arg {
    void *alloc_arg() { return nullptr; };
    void free_arg(void *p) {}
};

// struct nop : public no_arg {
//    void run(void *arg) {  }
//};

struct close_minus1 : public no_arg {
    void run(void *arg) { close(-1); }
};

struct open_close : public no_arg {
    void run(void *arg) {
        int fd = open("/", O_RDONLY);
        if (fd < 0) {
            perror("open");
            exit(1);
        }
        close(fd);
    }
};

struct pipe_close : public no_arg {
    void run(void *arg) {
        int fd[2];
        pipe(fd);
        close(fd[0]);
        close(fd[1]);
    }
};

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

struct write_devnull_1byte {
    struct arg {
        int fd;
    };

    struct arg *alloc_arg() {
        struct arg *a = new arg;
        a->fd = open("/dev/null", O_RDWR);
        if (a->fd == -1) {
            perror("/dev/null");
            exit(1);
        }

        return a;
    }

    void free_arg(struct arg *a) {
        close(a->fd);
        delete a;
    }

    void run(struct arg *a) {
        char x;
        ssize_t wrsz = write(a->fd, &x, 1);
        if (wrsz != 1) {
            perror("write");
            exit(1);
        }
    }
};

static void *thread_func(void *) { return nullptr; }

struct pthread_create_join : public no_arg {
    void run(void *arg) {
        pthread_t t;
        int r = pthread_create(&t, NULL, thread_func, nullptr);
        if (r != 0) {
            perror("pthread_create");
            exit(1);
        }

        pthread_join(t, NULL);
    }
};

struct gettimeofday1 : public no_arg {
    void run(void *arg) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
    }
};

struct clock_gettime1 : public no_arg {
    void run(void *arg) {
        struct timespec tv;
        clock_gettime(CLOCK_MONOTONIC, &tv);
    }
};

template <typename F> double run_test(const GlobalState *g, F *f) {
    auto a = f->alloc_arg();

    f->run(a);

    oneshot_timer ot(2);
    uint64_t count = 0;
    ot.start(g, 0.1);

    while (!ot.test_end()) {
        f->run(a);
        count++;
    }

    double dsec = ot.actual_interval_sec(g);
    double ret = (dsec / count) * 1e9;

    f->free_arg(a);

    return ret;
}

#define FOR_EACH_TEST(F)                                                       \
    /*F(nop)*/                                                                 \
    F(close_minus1)                                                            \
    F(open_close)                                                              \
    F(pipe_close)                                                              \
    F(select_0)                                                                \
    F(fork_wait)                                                               \
    F(mmap_unmap)                                                              \
    F(write_devnull_1byte)                                                     \
    F(pthread_create_join)                                                     \
    F(gettimeofday1)                                                           \
    F(clock_gettime1)

struct Syscall : public BenchDesc {
    Syscall() : BenchDesc("syscall") {}

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
    virtual result_t parse_json_result(picojson::value const &v) {
        return result_t(Table1D<double, std::string>::parse_json_result(v));
    }
};

std::unique_ptr<BenchDesc> get_syscall_desc() {
    return std::unique_ptr<BenchDesc>(new Syscall());
}

} // namespace smbm