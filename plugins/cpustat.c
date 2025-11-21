/*
 * cpustat plugin: per-core usage and where cpu time goes.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sysmon/plugin.h"

#define MAX_CPUS 256

typedef struct times {
    unsigned long long user, nice, sys, idle, iowait, irq, softirq, steal;
} times_t;

static const sysmon_host_t *H;
static times_t prev[MAX_CPUS + 1]; /* [0] is the aggregate */
static int ncpu;

static int read_times(times_t *cur)
{
    char line[1024], name[16];
    int n = 0;

    FILE *f = fopen("/proc/stat", "r");
    if (!f)
        return -1;
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, "cpu", 3) != 0) {
            continue;
        }
        times_t t;
        memset(&t, 0, sizeof t);
        if (sscanf(line, "%15s %llu %llu %llu %llu %llu %llu %llu %llu", name, &t.user, &t.nice,
                   &t.sys, &t.idle, &t.iowait, &t.irq, &t.softirq, &t.steal) < 5)
            continue;
        int idx = 0;
        if (name[3] != '\0') {
            idx = atoi(name + 3) + 1;
            if (idx > MAX_CPUS)
                continue;
            if (idx > n)
                n = idx;
        }
        cur[idx] = t;
    }
    fclose(f);
    return n;
}

static int cpustat_init(const sysmon_host_t *host)
{
    H = host;
    ncpu = read_times(prev);
    if (ncpu <= 0) {
        H->log(SYSMON_LOG_WARN, "cpustat: cannot read cpu times");
        return -1;
    }
    H->set("cpu_count", ncpu);
    return 0;
}

static int cpustat_collect(void)
{
    times_t cur[MAX_CPUS + 1];
    int n = read_times(cur);

    if (n <= 0)
        return -1;
    memcpy(prev, cur, sizeof(times_t) * (size_t)(n + 1));
    return 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "cpustat", "1.0", cpustat_init, cpustat_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
