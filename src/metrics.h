#ifndef SYSMON_METRICS_H
#define SYSMON_METRICS_H

#include <stddef.h>

#define METRIC_NAME_MAX 64
#define METRICS_MAX     256

/*
 * Flat registry of named gauges. Collectors and plugins call
 * metrics_set() on every tick; readers (socket, http) iterate.
 * Single-threaded by design: everything runs in the main loop.
 */
typedef struct metric {
    char name[METRIC_NAME_MAX];
    double value; /* NAN until the first sample */
} metric_t;

metric_t *metrics_set(const char *name, double value);
const metric_t *metrics_get(const char *name);
size_t metrics_count(void);
const metric_t *metrics_at(size_t i);

#endif
