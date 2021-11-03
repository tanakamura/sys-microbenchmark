#pragma once

#include "sys-microbenchmark.h"
#include "oneshot_timer.h"

namespace smbm {

namespace af {
template <typename E, typename F>
E op(E v0, E v1, E z, oneshot_timer *ot, F f)
{
    E a = v0;

    E A = v1;
    E B = A+v1;
    E C = B+v1;
    E D = C+v1;

    while(! ot->test_end()) {
        REP8(
            A = f(A,a);
            B = f(B,a);
            C = f(C,a);
            D = f(D,a);
            );

        a = a&z;
        a = a|C;
    }

    A = f(A,B);
    C = f(C,D);

    A = f(A,C);

    return A;
}

template <typename E, typename F>
E opf(E v0, E v1, E z, oneshot_timer *ot, F f)
{
    E a = v0;

    E A = v1;
    E B = A+v1;
    E C = B+v1;
    E D = C+v1;

    while(! ot->test_end()) {
        REP8(
            A = f(A,a);
            B = f(B,a);
            C = f(C,a);
            D = f(D,a);
            );
    }

    A = f(A,B);
    C = f(C,D);

    A = f(A,C);

    return A;
}

}

}
