#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "log.h"
#include "metrics.h"

static metric_t metrics[METRICS_MAX];
static size_t nmetrics;

static metric_t *find(const char *name)
{
    for (size_t i = 0; i < nmetrics; i++)
        if (strcmp(metrics[i].name, name) == 0)
            return &metrics[i];
    return NULL;
}

static metric_t *get_or_create(const char *name)
{
    metric_t *m = find(name);
    if (m)
        return m;
    if (nmetrics == METRICS_MAX) {
        log_warn("metrics: registry full, dropping '%s'", name);
        return NULL;
    }
    m = &metrics[nmetrics++];
    memset(m, 0, sizeof *m);
    snprintf(m->name, sizeof m->name, "%s", name);
    m->value = NAN;
    return m;
}

metric_t *metrics_set(const char *name, double value)
{
    metric_t *m = get_or_create(name);
    if (!m)
        return NULL;
    m->value = value;
    return m;
}

const metric_t *metrics_get(const char *name)
{
    return find(name);
}

size_t metrics_count(void)
{
    return nmetrics;
}

const metric_t *metrics_at(size_t i)
{
    return i < nmetrics ? &metrics[i] : NULL;
}
