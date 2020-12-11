#pragma once

#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include "sys-microbenchmark.h"

namespace smbm {

struct oneshot_timer {
    int count;
    int count_max;

    userland_timer_value tv_start;
    userland_timer_value tv_end;
    userland_timer_value tv_end_actual;

    oneshot_timer(int count)
        :count_max(count)
    {}

    void start(const GlobalState *g, double sec_delay) {
        tv_start = userland_timer_value::get();
        tv_end = g->inc_sec_userland_timer(&tv_start, sec_delay);
        count = 0;
    }

    bool test_end() {
        count++;
        if (count == count_max) {
            count = 0;
            tv_end_actual = userland_timer_value::get();

            if (tv_end_actual >= tv_end) {
                return true;
            }
        }

        return false;
    }

    double actual_interval_sec(const GlobalState *g) {
        return g->userland_timer_delta_to_sec(this->tv_end_actual - this->tv_start);
    };
};

} // namespace smbm