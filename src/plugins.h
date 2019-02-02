#ifndef SYSMON_PLUGINS_H
#define SYSMON_PLUGINS_H

#include "config.h"

/* Loader for the plugins listed in [plugins] (see include/sysmon/plugin.h). */
int plugins_load(const config_t *cfg);
void plugins_collect(void);
void plugins_unload(void);
int plugins_count(void);

#endif
