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
    double ret = (dsec / count) * 1e9;

    f->free_arg(a);

    return ret;
}

}