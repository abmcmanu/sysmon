/*
 * Micro benchmarks for the hot paths: `make bench`.
 * Numbers are only comparable on the same machine.
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "log.h"
#include "metrics.h"
#include "ring.h"

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

#define BENCH(label, iters, body)                                                    \
    do {                                                                             \
        double t0 = now();                                                           \
        for (long i_ = 0; i_ < (iters); i_++) {                                      \
            body;                                                                    \
        }                                                                            \
        double dt = now() - t0;                                                      \
        printf("%-28s %10.1f ns/op\n", label, dt * 1e9 / (double)(iters));           \
    } while (0)

static ring_t r;

int main(void)
{
    log_set_level(-1);
    BENCH("ring_push", 50000000L, ring_push(&r, (double)i_));

    char name[32];
    for (int i = 0; i < 200; i++) {
        snprintf(name, sizeof name, "metric_%d", i);
        metrics_set(name, i);
    }
    BENCH("metrics_set (200 metrics)", 5000000L, metrics_set("metric_199", (double)i_));
    return 0;
}
