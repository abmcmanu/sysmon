#ifndef SYSMON_CONFIG_H
#define SYSMON_CONFIG_H

#define CFG_PATH_MAX 256
#define CFG_LIST_MAX 16

typedef struct config {
    int interval;  /* seconds between two samples */
    int log_level;
    char log_file[CFG_PATH_MAX];
    char pid_file[CFG_PATH_MAX];
    char mounts[CFG_LIST_MAX][CFG_PATH_MAX];
    int nmounts;
} config_t;

void config_defaults(config_t *cfg);
/* Returns 0 on success, -1 if the file is missing or has errors. */
int config_load(config_t *cfg, const char *path);

#endif
