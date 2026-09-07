/*
 * conntrack plugin: netfilter connection tracking table (Linux).
 * A full table drops new connections, so it is worth watching on NAT
 * gateways and container hosts.
 */
#include <stdio.h>

#include "sysmon/plugin.h"

static const sysmon_host_t *H;

static int read_num(const char *path, double *out)
{
    unsigned long long v;
    FILE *f = fopen(path, "r");

    if (!f)
        return -1;
    int n = fscanf(f, "%llu", &v);
    fclose(f);
    if (n != 1)
        return -1;
    *out = (double)v;
    return 0;
}

static int conntrack_collect(void)
{
    double count, max;

    if (read_num("/proc/sys/net/netfilter/nf_conntrack_count", &count) < 0)
        return -1;
    H->set("conntrack_entries", count);
    if (read_num("/proc/sys/net/netfilter/nf_conntrack_max", &max) == 0 && max > 0) {
        H->set("conntrack_max", max);
        H->set("conntrack_used_percent", 100.0 * count / max);
    }
    return 0;
}

static int conntrack_init(const sysmon_host_t *host)
{
    H = host;
    if (conntrack_collect() < 0) {
        H->log(SYSMON_LOG_WARN, "conntrack: nf_conntrack is not loaded");
        return -1;
    }
    return 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "conntrack", "1.0", conntrack_init, conntrack_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
