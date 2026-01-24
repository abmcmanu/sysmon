#ifndef SYSMON_RING_H
#define SYSMON_RING_H

#include <stddef.h>

/*
 * Fixed-size circular buffer of samples. No allocation: one ring is
 * embedded in every metric, so memory use is bounded and known upfront
 * (RING_CAP * sizeof(double) * METRICS_MAX).
 */
#define RING_CAP 300

typedef struct ring {
    double v[RING_CAP];
    size_t head; /* next slot to write */
    size_t len;  /* number of valid samples, <= RING_CAP */
} ring_t;

void ring_push(ring_t *r, double x);
size_t ring_len(const ring_t *r);
/* i = 0 is the oldest sample, i = len - 1 the newest */
double ring_at(const ring_t *r, size_t i);
double ring_last(const ring_t *r);
/* average of the newest n samples */
double ring_avg(const ring_t *r, size_t n);
double ring_min(const ring_t *r, size_t n);
double ring_max(const ring_t *r, size_t n);

#endif
