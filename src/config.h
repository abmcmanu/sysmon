#ifndef SYSMON_CONFIG_H
#define SYSMON_CONFIG_H

#define CFG_PATH_MAX 256
#define CFG_LIST_MAX 16

typedef struct config {
    int interval;  /* seconds between two samples */
    int log_level;
} config_t;

void config_defaults(config_t *cfg);

#endif
