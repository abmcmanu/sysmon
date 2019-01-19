/*
 * sysmon plugin ABI.
 *
 * A plugin is a shared object exporting one symbol:
 *
 *     const sysmon_plugin_t *sysmon_plugin_entry(void);
 *
 * (use SYSMON_PLUGIN_EXPORT). The daemon dlopen()s it, checks `abi`,
 * calls init() once with a host vtable, then collect() on every tick
 * and fini() on shutdown. Everything runs on the daemon's main thread:
 * collect() must be quick and must not block.
 *
 * Plugins never link against sysmond. They only talk to it through the
 * function pointers in sysmon_host_t, so the daemon's internals can
 * change freely as long as this header does not. Any incompatible
 * change to these structs bumps SYSMON_PLUGIN_ABI.
 */
#ifndef SYSMON_PLUGIN_H
#define SYSMON_PLUGIN_H

#define SYSMON_PLUGIN_ABI 1

enum {
    SYSMON_LOG_ERR,
    SYSMON_LOG_WARN,
    SYSMON_LOG_INFO,
    SYSMON_LOG_DEBUG,
};

typedef struct sysmon_host {
    int abi;
    /* set a gauge; names must match [a-zA-Z_][a-zA-Z0-9_]* */
    void (*set)(const char *name, double value);
    void (*set_help)(const char *name, const char *help);
    void (*log)(int level, const char *fmt, ...)
        __attribute__((format(printf, 2, 3)));
} sysmon_host_t;

typedef struct sysmon_plugin {
    int abi;             /* SYSMON_PLUGIN_ABI */
    const char *name;
    const char *version;
    /* return 0 on success; non-zero unloads the plugin */
    int (*init)(const sysmon_host_t *host);
    /* return 0 on success; repeated failures disable the plugin */
    int (*collect)(void);
    void (*fini)(void);  /* may be NULL */
} sysmon_plugin_t;

typedef const sysmon_plugin_t *(*sysmon_plugin_entry_fn)(void);

#define SYSMON_PLUGIN_EXPORT(p) \
    const sysmon_plugin_t *sysmon_plugin_entry(void) { return &(p); }

#endif
