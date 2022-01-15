#ifndef SYSMON_JSON_H
#define SYSMON_JSON_H

#include "sb.h"

void json_string(sb_t *out, const char *s);
void json_number(sb_t *out, double v);
/* {"version": ..., "timestamp": ..., "metrics": {"name": value, ...}} */
void json_metrics(sb_t *out);

#endif
