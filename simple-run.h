#pragma once

namespace smbm {

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
    double ret = (dsec / (count)) * 1e9;

    f->free_arg(a);

    return ret;
}

template <typename F> double run_test_g(const GlobalState *g, F *f) {
    auto a = f->alloc_arg();

    f->run(g, a);

    oneshot_timer ot(2);
    uint64_t count = 0;
    ot.start(g, 0.1);

    while (!ot.test_end()) {
        f->run(g, a);
        count++;
    }

    double dsec = ot.actual_interval_sec(g);
    double ret = (dsec / (count)) * 1e9;

    f->free_arg(a);

    return ret;
}

template <typename F> double run_test_unroll32(const GlobalState *g, F *f) {
    auto a = f->alloc_arg();

    f->run(a);

    oneshot_timer ot(2);
    uint64_t count = 0;
    ot.start(g, 0.1);

    while (!ot.test_end()) {
        REP16(f->run(a););
        REP16(f->run(a););
        count++;
    }

    double dsec = ot.actual_interval_sec(g);
    double ret = (dsec / (count*32)) * 1e9;

    f->free_arg(a);

    return ret;
}

}
