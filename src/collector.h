#ifndef SYSMON_COLLECTOR_H
#define SYSMON_COLLECTOR_H

#include "config.h"

/*
 * Platform collector: one implementation per OS (collect_linux.c,
 * collect_darwin.c), selected by the Makefile.
 */
int collect_init(void);
int collect_sample(void);

/* Portable (statvfs) disk usage of the configured mount points. */
int collect_disks(const config_t *cfg);

#endif
