#include "sys-microbenchmark.h"
#include <unistd.h>
#include "oneshot_timer.h"

int
main()
{
    using namespace smbm;

    GlobalState g;

    auto t0 = userland_timer_value::get();
    sleep(1);
    auto t1 = userland_timer_value::get();

    double dt = g.userland_timer_delta_to_sec(t1-t0);

    double dexpect = fabs(dt - 1.0);
    if (dexpect > 1e-3) {
        printf("too large error @ get_cputime delta=%f, %f-%f\n", dexpect, dt, 1e9);
        exit(1);
    }

    {
        oneshot_timer ot(1);
        auto t0 = ostimer_value::get();
        double delay = 0.5;

        ot.start(&g, delay);
        while(!ot.test_end()) {
            
        }
        auto t1 = ostimer_value::get();

        double delta_sec = g.ostimer_delta_to_sec(t1-t0);

        double delta_from_expect = fabs(delta_sec - delay);

        if (delta_from_expect > 1e-3) {
            printf("too large error @ oneshot timer = %e\n", delta_from_expect);
            exit(1);
        }
    }
    
}

