#ifndef SYSMON_ALERT_H
#define SYSMON_ALERT_H

#include <time.h>

#include "sb.h"

/*
 * Threshold alerts:  METRIC OP THRESHOLD [for N]
 *
 *   cpu_usage_percent > 90 for 3     3 consecutive samples above 90
 *   disk_root_used_percent >= 95
 *
 * OP is one of > >= < <=. An alert fires once the condition held for
 * N consecutive samples and resolves as soon as it stops holding.
 */
enum { ALERT_GT, ALERT_GE, ALERT_LT, ALERT_LE };

typedef struct alert {
    char rule[256];
    char metric[64];
    int op;
    double threshold;
    int for_ticks;
    int for_seconds; /* "for 30s": converted to ticks with the interval */
    int count;     /* consecutive samples matching */
    int firing;
    time_t since;  /* last state change */
    double last;   /* last value seen */
} alert_t;

int alert_parse(const char *rule, alert_t *a);
/* Feeds one sample. Returns 1 when the alert starts firing,
 * -1 when it resolves, 0 otherwise. */
int alert_update(alert_t *a, double value);

int alerts_load(char rules[][256], int n, const char *exec_path);
void alerts_eval(void);
int alerts_firing(void);
void alerts_dump(sb_t *out);

#endif
