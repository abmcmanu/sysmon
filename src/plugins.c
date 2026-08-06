#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "log.h"
#include "metrics.h"
#include "plugins.h"
#include "sysmon/plugin.h"

#define MAX_FAILURES 5

typedef struct loaded {
    void *handle;
    const sysmon_plugin_t *p;
    int failures;
    int disabled;
} loaded_t;

#define SLOW_MS 100.0

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e3 + (double)ts.tv_nsec / 1e6;
}

static loaded_t loaded[CFG_LIST_MAX];
static int nloaded;
static const config_t *config;

static void host_set(const char *name, double value)
{
    metrics_set(name, value);
}

static void host_set_help(const char *name, const char *help)
{
    metrics_set_help(name, help);
}

static void host_log(int level, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_vmsg(level, fmt, ap); /* SYSMON_LOG_* == SM_LOG_* */
    va_end(ap);
}

static const char *host_option(const char *plugin, const char *key)
{
    for (int i = 0; config && i < config->nopts; i++)
        if (strcmp(config->opts[i].plugin, plugin) == 0 && strcmp(config->opts[i].key, key) == 0)
            return config->opts[i].value;
    return NULL;
}

static const sysmon_host_t host = {
    SYSMON_PLUGIN_ABI, host_set, host_set_help, host_log, host_option,
};

static int load_one(const char *dir, const char *name)
{
    char path[512];
    snprintf(path, sizeof path, "%s/%s.so", dir, name);

    void *h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!h) {
        log_err("plugin %s: %s", name, dlerror());
        return -1;
    }

    /* POSIX-blessed way to turn a void * into a function pointer */
    sysmon_plugin_entry_fn entry;
    *(void **)(&entry) = dlsym(h, "sysmon_plugin_entry");
    const sysmon_plugin_t *p = entry ? entry() : NULL;
    if (!p) {
        log_err("plugin %s: no sysmon_plugin_entry symbol", name);
        dlclose(h);
        return -1;
    }
    if (p->abi < 1 || p->abi > SYSMON_PLUGIN_ABI) {
        log_err("plugin %s: ABI %d, daemon speaks 1 to %d", name, p->abi, SYSMON_PLUGIN_ABI);
        dlclose(h);
        return -1;
    }
    if (p->init && p->init(&host) != 0) {
        log_warn("plugin %s: init failed, not loaded", name);
        dlclose(h);
        return -1;
    }

    memset(&loaded[nloaded], 0, sizeof loaded[nloaded]);
    loaded[nloaded].handle = h;
    loaded[nloaded].p = p;
    nloaded++;
    log_info("plugin %s %s loaded", p->name, p->version ? p->version : "");
    return 0;
}

int plugins_load(const config_t *cfg)
{
    int errors = 0;
    config = cfg;
    for (int i = 0; i < cfg->nplugins && nloaded < CFG_LIST_MAX; i++)
        if (load_one(cfg->plugin_dir, cfg->plugins[i]) < 0)
            errors++;
    return errors ? -1 : 0;
}

void plugins_collect(void)
{
    for (int i = 0; i < nloaded; i++) {
        loaded_t *l = &loaded[i];
        if (l->disabled || !l->p->collect)
            continue;
        double t0 = now_ms();
        if (l->p->collect() == 0) {
            l->failures = 0;
        } else if (++l->failures >= MAX_FAILURES) {
            l->disabled = 1;
            log_warn("plugin %s: %d consecutive failures, disabled", l->p->name, MAX_FAILURES);
        }
        double ms = now_ms() - t0;
        if (ms > SLOW_MS)
            log_warn("plugin %s: collect took %.0f ms, it delays every sample", l->p->name, ms);
    }
}

void plugins_unload(void)
{
    for (int i = 0; i < nloaded; i++) {
        if (loaded[i].p->fini)
            loaded[i].p->fini();
        dlclose(loaded[i].handle);
    }
    nloaded = 0;
}

int plugins_count(void)
{
    return nloaded;
}
