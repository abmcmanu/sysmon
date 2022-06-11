#ifndef SYSMON_CONFIG_H
#define SYSMON_CONFIG_H

#define CFG_PATH_MAX 256
#define CFG_LIST_MAX 16
#define CFG_ALERTS_MAX 32
#define CFG_RULE_MAX 256

typedef struct config {
    int interval;  /* seconds between two samples */
    int log_level;
    int print;     /* -p: print samples to stdout */
    char log_file[CFG_PATH_MAX];
    char pid_file[CFG_PATH_MAX];
    char mounts[CFG_LIST_MAX][CFG_PATH_MAX];
    int nmounts;
    char socket_path[108]; /* sizeof(sun_path) on Linux */
    char plugin_dir[CFG_PATH_MAX];
    char plugins[CFG_LIST_MAX][64];
    int nplugins;
    char http_bind[64];
    int http_port; /* 0 = disabled */
    char alerts[CFG_ALERTS_MAX][CFG_RULE_MAX];
    int nalerts;
    char alert_exec[CFG_PATH_MAX];
} config_t;

void config_defaults(config_t *cfg);
/* Returns 0 on success, -1 if the file is missing or has errors. */
int config_load(config_t *cfg, const char *path);

#endif
