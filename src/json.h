#ifndef SYSMON_JSON_H
#define SYSMON_JSON_H

#include "sb.h"

void json_string(sb_t *out, const char *s);
void json_number(sb_t *out, double v);
/* {"version": ..., "timestamp": ..., "metrics": {"name": value, ...}} */
void json_metrics(sb_t *out);

#include <stddef.h>

#include "metrics.h"

/* {"metric": NAME, "values": [oldest, ..., newest]} */
void json_history(sb_t *out, const metric_t *m, size_t n);

#endif
