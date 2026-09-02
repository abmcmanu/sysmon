/*
 * sockets plugin: socket counts from /proc/net/sockstat (Linux).
 */
#include <stdio.h>
#include <string.h>

#include "sysmon/plugin.h"

static const sysmon_host_t *H;

static int read_sockstat(const char *path, int v6)
{
    char line[256];
    FILE *f = fopen(path, "r");
    unsigned long long inuse, orphan, tw, alloc;

    if (!f)
        return -1;
    while (fgets(line, sizeof line, f)) {
        if (!v6 && sscanf(line, "TCP: inuse %llu orphan %llu tw %llu alloc %llu", &inuse, &orphan,
                          &tw, &alloc) == 4) {
            H->set("tcp_inuse", (double)inuse);
            H->set("tcp_orphan", (double)orphan);
            H->set("tcp_time_wait", (double)tw);
        } else if (!v6 && sscanf(line, "UDP: inuse %llu", &inuse) == 1) {
            H->set("udp_inuse", (double)inuse);
        } else if (v6 && sscanf(line, "TCP6: inuse %llu", &inuse) == 1) {
            H->set("tcp6_inuse", (double)inuse);
        } else if (v6 && sscanf(line, "UDP6: inuse %llu", &inuse) == 1) {
            H->set("udp6_inuse", (double)inuse);
        }
    }
    fclose(f);
    return 0;
}

static int sockets_init(const sysmon_host_t *host)
{
    H = host;
    if (read_sockstat("/proc/net/sockstat", 0) < 0) {
        H->log(SYSMON_LOG_WARN, "sockets: /proc/net/sockstat not available");
        return -1;
    }
    return 0;
}

static int sockets_collect(void)
{
    if (read_sockstat("/proc/net/sockstat", 0) < 0)
        return -1;
    read_sockstat("/proc/net/sockstat6", 1); /* absent without IPv6 */
    return 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "sockets", "1.0", sockets_init, sockets_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
