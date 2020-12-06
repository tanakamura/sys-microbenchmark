#pragma once

#include <sys/time.h>
#include <time.h>
#include <stdint.h>

namespace smbm {

struct oneshot_timer {
    int count;
    int count_max;
    struct timespec tv_start;
    struct timespec tv_end;
    struct timespec tv_end_actual;

    oneshot_timer(int count)
        :count_max(count)
    {}

    static constexpr int64_t NANO = 1000000000;

    void start(int msec_delay) {
        int sec_dealy = msec_delay / 1000;
        msec_delay = msec_delay % 1000;

        count = 0;
        clock_gettime(CLOCK_MONOTONIC, &tv_start);

        tv_end.tv_sec = tv_start.tv_sec + sec_dealy;

        tv_end.tv_nsec = tv_start.tv_nsec;
        tv_end.tv_nsec += msec_delay * 1000000;

        if (tv_end.tv_nsec > NANO) {
            tv_end.tv_sec++;
            tv_end.tv_nsec -= NANO;
        }
    }

    bool test_end() {
        count++;
        if (count == count_max) {
            count = 0;
            clock_gettime(CLOCK_MONOTONIC, &tv_end_actual);

            if (tv_end_actual.tv_sec > tv_end.tv_sec) {
                return true;
            }

            if (tv_end_actual.tv_sec == tv_end.tv_sec) {
                if (tv_end_actual.tv_nsec >= tv_end.tv_nsec) {
                    return true;
                }
            }
        }

        return false;
    }

    double actual_interval() {
        int64_t nsec_delta = tv_end.tv_nsec - tv_start.tv_nsec;
        int64_t sec_delta = tv_end.tv_sec - tv_start.tv_sec;

        if (nsec_delta < 0) {
            sec_delta --;
            nsec_delta += 1000000000;
        }

        return sec_delta + (nsec_delta/1000000000.0);
    };
};

} // namespace smbm