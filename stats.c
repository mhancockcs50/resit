#include "stats.h"

#include <math.h>

/* The routines below walk the buffer with a pointer rather than indexing.
 * `p` steps one double (8 bytes) forward each iteration; `end` is one past
 * the last element so the loop stops exactly when it should. This is the
 * pointer arithmetic the brief asks to see demonstrated. */

double stats_mean(const double *xs, size_t n)
{
    if (n == 0) return 0.0;

    const double *end = xs + n;
    double sum = 0.0;
    for (const double *p = xs; p != end; ++p)
        sum += *p;
    return sum / (double)n;
}

double stats_min(const double *xs, size_t n)
{
    if (n == 0) return 0.0;
    const double *end = xs + n;
    double m = *xs;
    for (const double *p = xs + 1; p != end; ++p)
        if (*p < m) m = *p;
    return m;
}

double stats_max(const double *xs, size_t n)
{
    if (n == 0) return 0.0;
    const double *end = xs + n;
    double m = *xs;
    for (const double *p = xs + 1; p != end; ++p)
        if (*p > m) m = *p;
    return m;
}

double stats_stddev(const double *xs, size_t n)
{
    if (n < 2) return 0.0;

    const double *end = xs + n;

    /* pass 1: mean */
    double sum = 0.0;
    for (const double *p = xs; p != end; ++p)
        sum += *p;
    double mean = sum / (double)n;

    /* pass 2: sum of squared deviations */
    double acc = 0.0;
    for (const double *p = xs; p != end; ++p) {
        double d = *p - mean;
        acc += d * d;
    }
    return sqrt(acc / (double)n);
}

double stats_rms(const double *xs, size_t n)
{
    if (n == 0) return 0.0;
    const double *end = xs + n;
    double acc = 0.0;
    for (const double *p = xs; p != end; ++p)
        acc += *p * *p;
    return sqrt(acc / (double)n);
}
