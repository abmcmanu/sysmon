#ifndef SYSMON_COLLECTOR_H
#define SYSMON_COLLECTOR_H

/*
 * Platform collector: one implementation per OS (collect_linux.c,
 * more to come), selected by the Makefile.
 */
int collect_init(void);
int collect_sample(void);

#endif
