#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "log.h"
#include "metrics.h"

static metric_t metrics[METRICS_MAX];
static size_t nmetrics;

int metric_name_valid(const char *name)
{
    if (!name || !*name || strlen(name) >= METRIC_NAME_MAX)
        return 0;
    if (!isalpha((unsigned char)name[0]) && name[0] != '_')
        return 0;
    for (const char *p = name + 1; *p; p++)
        if (!isalnum((unsigned char)*p) && *p != '_')
            return 0;
    return 1;
}

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
    if (!metric_name_valid(name)) {
        log_warn("metrics: invalid metric name '%s', ignored", name);
        return NULL;
    }
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
    ring_push(&m->hist, value);
    return m;
}

void metrics_set_help(const char *name, const char *help)
{
    metric_t *m = get_or_create(name);
    if (m)
        snprintf(m->help, sizeof m->help, "%s", help);
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
