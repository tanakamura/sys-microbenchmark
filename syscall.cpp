#include "oneshot_timer.h"
#include "sys-microbenchmark.h"
#include "table.h"
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/mman.h>

namespace smbm {

// struct syscall_func {
//    virtual void *alloc_arg() = 0;
//    virtual void free_arg(void *) = 0;
//    virtual double run1(void *arg, int msec) = 0;
//};

struct no_arg {
    void *alloc_arg() { return nullptr; };
    void free_arg(void *p) {}
};

//struct nop : public no_arg {
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
        fd_set rd,wr,except;
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
    void run(void *arg) {
        pid_t c = fork();
        if (c == 0) {
            exit(0);
        }
        int st;
        wait(&st);
    }
};

struct mmap_unmap : public no_arg {
    void run(void *arg) {
        void *p = mmap(0, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);
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

template <typename F> double run_test(F *f) {
    auto a = f->alloc_arg();

    f->run(a);

    oneshot_timer ot(2048);
    uint64_t count = 0;
    ot.start(100);

    while (!ot.test_end()) {
        f->run(a);
        count++;
    }

    double d = ot.actual_interval();
    double ret = count / (d * 1e6);

    f->free_arg(a);

    return ret;
}

#define FOR_EACH_TEST(F)                                                       \
    /*F(nop)*/                                                          \
    F(close_minus1)                                                               \
    F(open_close)                                                              \
    F(pipe_close)                                                       \
    F(select_0)                                                         \
    F(fork_wait)                                                        \
    F(mmap_unmap)                                                             \
    F(write_devnull_1byte)                                              \
    F(pthread_create_join)                                                        \
    F(gettimeofday1)                                                     \
    F(clock_gettime1)                                                    \

static std::unique_ptr<BenchResult> run(std::ostream &os) {
    int count = 0;
#define INC_COUNT(F) count++;
    FOR_EACH_TEST(INC_COUNT);

    typedef Table1D<double, std::string> result_t;
    result_t *result = new result_t("test_name", count);

#define NAME(F) #F,

    result->column_label = " MCall/sec";
    result->row_label = std::vector<std::string>{FOR_EACH_TEST(NAME)};

    count = 0;

#define RUN(F)                                                                 \
    {                                                                          \
        F t;                                                                   \
        (*result)[count++] = run_test(&t);                                     \
    }
    FOR_EACH_TEST(RUN)

    dump1d(os, result);

    return std::unique_ptr<BenchResult>(result);
}

BenchDesc get_syscall_desc() {
    BenchDesc ret;
    ret.name = "syscall";
    ret.run = run;

    return ret;
}

} // namespace smbm