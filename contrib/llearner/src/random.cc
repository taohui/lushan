#include "random.h"
#include <cmath>
#include <stdlib.h>

double ran_uniform() {
    return rand()/((double)RAND_MAX + 1);
}

double ran_gaussian() {
    // Joseph L. Leva: A fast normal Random number generator
    double u,v, x, y, Q;
    do {
        do {
            u = ran_uniform();
        } while (u == 0.0);
        v = 1.7156 * (ran_uniform() - 0.5);
        x = u - 0.449871;
        y = std::abs(v) + 0.386595;
        Q = x*x + y*(0.19600*y-0.25472*x);
        if (Q < 0.27597) { break; }
    } while ((Q > 0.27846) || ((v*v) > (-4.0*u*u*std::log(u))));
    return v / u;
}

double ran_gaussian(double mean, double stdev) {
    if ((stdev == 0.0) || (std::isnan(stdev))) {
        return mean;
    } else {
        return mean + stdev*ran_gaussian();
    }
}

