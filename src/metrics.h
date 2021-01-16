#ifndef SYSMON_METRICS_H
#define SYSMON_METRICS_H

#include <stddef.h>

#include "ring.h"

#define METRIC_NAME_MAX 64
#define METRIC_HELP_MAX 128
#define METRICS_MAX     256

/*
 * Flat registry of named gauges. Collectors and plugins call
 * metrics_set() on every tick; readers (socket, http) iterate.
 * Single-threaded by design: everything runs in the main loop.
 */
typedef struct metric {
    char name[METRIC_NAME_MAX];
    char help[METRIC_HELP_MAX];
    double value; /* NAN until the first sample */
    ring_t hist;
} metric_t;

/* [a-zA-Z_][a-zA-Z0-9_]*, the Prometheus metric name charset */
int metric_name_valid(const char *name);
metric_t *metrics_set(const char *name, double value);
void metrics_set_help(const char *name, const char *help);
const metric_t *metrics_get(const char *name);
size_t metrics_count(void);
const metric_t *metrics_at(size_t i);

#endif
