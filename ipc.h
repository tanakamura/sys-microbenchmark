#pragma once

#include <unistd.h>
#include "barrier.h"

namespace smbm {

typedef int pipe_handle_t;

struct Pipe {
    pipe_handle_t read_pipe;
    pipe_handle_t write_pipe;

    Pipe() {
        int fd[2];
        pipe(fd);

        this->read_pipe = fd[0];
        this->write_pipe = fd[1];
    }

    ~Pipe() {
        close(this->read_pipe);
        close(this->write_pipe);
    }

    Pipe(Pipe const &rhs) = delete;
    Pipe & operator = (Pipe const &rhs) = delete;
};

char read_pipe(Pipe *p) {
    char x;
    ssize_t rdsz = read(p->read_pipe, &x, 1);
    if (rdsz != 1) {
        abort();
    }

    rmb();

    return x;
}

void write_pipe(Pipe *p, char x) {
    wmb();

    ssize_t wrsz = write(p->write_pipe, &x, 1);
    if (wrsz != 1) {
        abort();
    }
            
}


};
