#ifndef SYSMON_GRAPHITE_H
#define SYSMON_GRAPHITE_H

#include <time.h>

#include "sb.h"

/*
 * Push every metric to Graphite (carbon plaintext protocol over UDP)
 * once per tick: "PREFIX.NAME VALUE TIMESTAMP\n".
 */
/* one line per metric; NaN (no sample yet) is skipped */
void graphite_format(sb_t *out, const char *prefix, time_t ts);

#include "config.h"

int graphite_open(const config_t *cfg);
void graphite_send(void);
void graphite_close(void);

#endif
