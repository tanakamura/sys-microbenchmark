#include "sys-microbenchmark.h"
#include <unistd.h>
#include "oneshot_timer.h"

double getsec() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int
main()
{
    using namespace smbm;

    GlobalState g(false);

    auto t0 = g.get_cputime();
    sleep(1);
    auto t1 = g.get_cputime();

    double dt = g.delta_cputime(&t1, &t0);

    double dexpect = fabs(dt - 1e9);
    if (dexpect > 1e6) {
        printf("too large error @ get_cputime delta=%f, %f-%f\n", dexpect, dt, 1e9);
        exit(1);
    }

    {
        oneshot_timer ot(1024);
        double t0 = getsec();

        ot.start(&g, 0.1);
        while(!ot.test_end()) {
            
        }
        double t1 = getsec();

        double dexpect = fabs((t1-t0) - 0.1);

        if (dexpect > 1e-3) {
            printf("too large error @ oneshot timer = %f\n", dexpect);
            exit(1);
        }
    }
    
}