#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alert.h"
#include "log.h"
#include "metrics.h"

#define ALERTS_MAX 32

static alert_t alerts[ALERTS_MAX];
static int nalerts;

int alert_parse(const char *rule, alert_t *a)
{
    char op[3] = "";
    int consumed = 0;

    memset(a, 0, sizeof *a);
    snprintf(a->rule, sizeof a->rule, "%s", rule);
    a->for_ticks = 1;
    a->last = NAN;

    if (sscanf(rule, "%63s %2s %lf%n", a->metric, op, &a->threshold, &consumed) != 3)
        return -1;

    if (strcmp(op, ">") == 0)
        a->op = ALERT_GT;
    else if (strcmp(op, ">=") == 0)
        a->op = ALERT_GE;
    else if (strcmp(op, "<") == 0)
        a->op = ALERT_LT;
    else if (strcmp(op, "<=") == 0)
        a->op = ALERT_LE;
    else
        return -1;

    const char *rest = rule + consumed;
    while (isspace((unsigned char)*rest))
        rest++;
    if (*rest) {
        int ticks;
        char extra;
        if (sscanf(rest, "for %d %c", &ticks, &extra) != 1 || ticks < 1)
            return -1;
        a->for_ticks = ticks;
    }
    return 0;
}

static int matches(const alert_t *a, double v)
{
    switch (a->op) {
    case ALERT_GT: return v > a->threshold;
    case ALERT_GE: return v >= a->threshold;
    case ALERT_LT: return v < a->threshold;
    case ALERT_LE: return v <= a->threshold;
    }
    return 0;
}

int alert_update(alert_t *a, double v)
{
    a->last = v;
    if (isnan(v))
        return 0;

    int hit = matches(a, v);
    if (hit && a->count < a->for_ticks)
        a->count++;
    else if (!hit)
        a->count = 0;

    if (!a->firing && a->count >= a->for_ticks) {
        a->firing = 1;
        a->since = time(NULL);
        return 1;
    }
    if (a->firing && !hit) {
        a->firing = 0;
        a->since = time(NULL);
        return -1;
    }
    return 0;
}

int alerts_load(char rules[][256], int n)
{
    alert_t parsed[ALERTS_MAX];
    int count = 0;

    for (int i = 0; i < n && count < ALERTS_MAX; i++) {
        if (alert_parse(rules[i], &parsed[count]) < 0) {
            log_err("alerts: cannot parse rule '%s'", rules[i]);
            return -1;
        }
        /* keep the state of rules that survive a reload */
        for (int j = 0; j < nalerts; j++) {
            if (strcmp(alerts[j].rule, parsed[count].rule) == 0) {
                parsed[count] = alerts[j];
                break;
            }
        }
        count++;
    }
    memcpy(alerts, parsed, sizeof(alert_t) * (size_t)count);
    nalerts = count;
    if (nalerts)
        log_info("alerts: %d rule(s) loaded", nalerts);
    return 0;
}

void alerts_eval(void)
{
    for (int i = 0; i < nalerts; i++) {
        alert_t *a = &alerts[i];
        const metric_t *m = metrics_get(a->metric);
        if (!m)
            continue;
        int t = alert_update(a, m->value);
        if (t > 0)
            log_warn("ALERT FIRING: %s (value %.6g)", a->rule, a->last);
        else if (t < 0)
            log_info("alert resolved: %s (value %.6g)", a->rule, a->last);
    }
}

int alerts_firing(void)
{
    int n = 0;
    for (int i = 0; i < nalerts; i++)
        n += alerts[i].firing;
    return n;
}

void alerts_dump(sb_t *out)
{
    if (nalerts == 0) {
        sb_puts(out, "no alert rules configured\n");
        return;
    }
    time_t now = time(NULL);
    for (int i = 0; i < nalerts; i++) {
        const alert_t *a = &alerts[i];
        sb_printf(out, "%-8s %s  (value %.6g", a->firing ? "FIRING" : "ok", a->rule, a->last);
        if (a->since)
            sb_printf(out, ", since %lds", (long)(now - a->since));
        sb_puts(out, ")\n");
    }
}
