#ifndef STATS_H
#define STATS_H

#include <stddef.h>

/* Generic statistics for an array of doubles. */

double stats_mean(const double *xs, size_t n);
double stats_min(const double *xs, size_t n);
double stats_max(const double *xs, size_t n);

/* Calculate the population standard deviation. */
double stats_stddev(const double *xs, size_t n);

/* Calculate the root-mean-square value. */
double stats_rms(const double *xs, size_t n);

#endif /* STATS_H */ 