/*
 * sockets plugin: socket counts from /proc/net/sockstat (Linux).
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "sysmon/plugin.h"

static const sysmon_host_t *H;
static unsigned long long prev_retrans;
static double prev_t;

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/* /proc/net/snmp has a header line and a value line per protocol */
static int read_retrans(unsigned long long *out)
{
    char head[1024], vals[1024];
    FILE *f = fopen("/proc/net/snmp", "r");
    int found = -1;

    if (!f)
        return -1;
    while (fgets(head, sizeof head, f) && fgets(vals, sizeof vals, f)) {
        if (strncmp(head, "Tcp:", 4) != 0)
            continue;
        char *hs, *vs;
        char *h = strtok_r(head, " \n", &hs), *v = strtok_r(vals, " \n", &vs);
        while (h && v) {
            if (strcmp(h, "RetransSegs") == 0 && sscanf(v, "%llu", out) == 1)
                found = 0;
            h = strtok_r(NULL, " \n", &hs);
            v = strtok_r(NULL, " \n", &vs);
        }
        break;
    }
    fclose(f);
    return found;
}

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
    if (read_retrans(&prev_retrans) == 0)
        prev_t = now();
    return 0;
}

static int sockets_collect(void)
{
    if (read_sockstat("/proc/net/sockstat", 0) < 0)
        return -1;
    read_sockstat("/proc/net/sockstat6", 1); /* absent without IPv6 */
    unsigned long long r;
    double t = now();
    if (read_retrans(&r) == 0) {
        if (prev_t > 0 && t > prev_t && r >= prev_retrans)
            H->set("tcp_retransmits_per_second", (double)(r - prev_retrans) / (t - prev_t));
        prev_retrans = r;
        prev_t = t;
    }
    return 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "sockets", "1.0", sockets_init, sockets_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
