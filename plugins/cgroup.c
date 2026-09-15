/*
 * cgroup plugin: resource usage of one cgroup v2 group, by default the
 * one sysmond runs in. Inside a container that is the container itself.
 *
 *   [plugin.cgroup]
 *   path = /sys/fs/cgroup/system.slice/nginx.service
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "sysmon/plugin.h"

static const sysmon_host_t *H;
static char base[256];

static int read_value(const char *file, unsigned long long *out)
{
    char path[384], buf[64];
    FILE *f;

    snprintf(path, sizeof path, "%s/%s", base, file);
    if (!(f = fopen(path, "r")))
        return -1;
    char *ok = fgets(buf, sizeof buf, f);
    fclose(f);
    if (!ok || strncmp(buf, "max", 3) == 0) /* "max" means no limit */
        return -1;
    return sscanf(buf, "%llu", out) == 1 ? 0 : -1;
}

/* "0::/user.slice/..." -> /sys/fs/cgroup/user.slice/... */
static int own_cgroup(void)
{
    char line[200];
    FILE *f = fopen("/proc/self/cgroup", "r");
    int found = -1;

    if (!f)
        return -1;
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, "0::", 3) == 0) {
            line[strcspn(line, "\n")] = '\0';
            const char *rel = strcmp(line + 3, "/") ? line + 3 : "";
            if (snprintf(base, sizeof base, "/sys/fs/cgroup%s", rel) < (int)sizeof base)
                found = 0;
        }
    }
    fclose(f);
    return found;
}

static int cgroup_init(const sysmon_host_t *host)
{
    unsigned long long v;

    H = host;
    snprintf(base, sizeof base, "/sys/fs/cgroup");
    own_cgroup();
    const char *p = H->abi >= 2 ? H->option("cgroup", "path") : NULL;
    if (p)
        snprintf(base, sizeof base, "%s", p);
    if (read_value("memory.current", &v) < 0 && read_value("pids.current", &v) < 0) {
        H->log(SYSMON_LOG_WARN, "cgroup: %s is not a cgroup v2 group", base);
        return -1;
    }
    H->log(SYSMON_LOG_INFO, "cgroup: watching %s", base);
    return 0;
}

static int cgroup_collect(void)
{
    unsigned long long cur, max;

    if (read_value("memory.current", &cur) == 0) {
        H->set("cgroup_memory_bytes", (double)cur);
        if (read_value("memory.max", &max) == 0 && max > 0) {
            H->set("cgroup_memory_limit_bytes", (double)max);
            H->set("cgroup_memory_percent", 100.0 * (double)cur / (double)max);
        }
    }
    return 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "cgroup", "1.0", cgroup_init, cgroup_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
