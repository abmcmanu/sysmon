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
#include <unistd.h>

#include "sysmon/plugin.h"

static const sysmon_host_t *H;
static char base[256];
static unsigned long long prev_usage, prev_throttled;
static double prev_t;
static long ncpu = 1;

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

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

/* "usage_usec 123\nuser_usec ..." */
static int read_stat(const char *file, const char *key, unsigned long long *out)
{
    char path[384], line[128], k[64];
    unsigned long long v;
    FILE *f;
    int found = -1;

    snprintf(path, sizeof path, "%s/%s", base, file);
    if (!(f = fopen(path, "r")))
        return -1;
    while (fgets(line, sizeof line, f))
        if (sscanf(line, "%63s %llu", k, &v) == 2 && strcmp(k, key) == 0) {
            *out = v;
            found = 0;
        }
    fclose(f);
    return found;
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
    ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    if (ncpu < 1)
        ncpu = 1;
    read_stat("cpu.stat", "usage_usec", &prev_usage);
    read_stat("cpu.stat", "nr_throttled", &prev_throttled);
    prev_t = now();
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
    unsigned long long usage, throttled;
    double t = now(), dt = t - prev_t;
    if (read_stat("cpu.stat", "usage_usec", &usage) == 0 && dt > 0 && usage >= prev_usage) {
        double pct = 100.0 * (double)(usage - prev_usage) / (dt * 1e6 * (double)ncpu);
        H->set("cgroup_cpu_usage_percent", pct);
        prev_usage = usage;
    }
    if (read_stat("cpu.stat", "nr_throttled", &throttled) == 0 && dt > 0 &&
        throttled >= prev_throttled) {
        H->set("cgroup_cpu_throttled_per_second", (double)(throttled - prev_throttled) / dt);
        prev_throttled = throttled;
    }
    prev_t = t;
    return 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "cgroup", "1.0", cgroup_init, cgroup_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
