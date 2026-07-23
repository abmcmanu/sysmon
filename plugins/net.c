/*
 * net plugin: receive/transmit throughput summed over all
 * non-loopback interfaces.
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/types.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>
#endif

#include "sysmon/plugin.h"

static const sysmon_host_t *H;
static unsigned long long prev_rx, prev_tx;
static double prev_t;
static char only[256];

/* is `name` in the comma separated list `only`? An empty list means all. */
static int wanted(const char *name)
{
    size_t n = strlen(name);

    if (!only[0])
        return 1;
    for (const char *p = only; *p;) {
        while (*p == ' ' || *p == ',')
            p++;
        size_t len = strcspn(p, ", ");
        if (len == n && strncmp(p, name, n) == 0)
            return 1;
        p += len;
    }
    return 0;
}

/* container and VPN plumbing: counting it twice inflates the totals */
static int virtual_if(const char *name)
{
    static const char *const skip[] = { "veth", "docker", "br-", "virbr", "cni", "flannel",
                                        "vxlan", "utun", "awdl", "llw", "bridge", "gif", "stf" };

    for (size_t i = 0; i < sizeof skip / sizeof skip[0]; i++)
        if (strncmp(name, skip[i], strlen(skip[i])) == 0)
            return 1;
    return 0;
}

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

#if defined(__APPLE__) || defined(__FreeBSD__)
static int read_counters(unsigned long long *rx, unsigned long long *tx)
{
    struct ifaddrs *ifa;
    if (getifaddrs(&ifa) < 0)
        return -1;
    *rx = *tx = 0;
    for (struct ifaddrs *i = ifa; i; i = i->ifa_next) {
        if (!i->ifa_addr || i->ifa_addr->sa_family != AF_LINK || !i->ifa_data)
            continue;
        if (i->ifa_flags & IFF_LOOPBACK)
            continue;
        if (!wanted(i->ifa_name))
            continue;
        if (!only[0] && virtual_if(i->ifa_name))
            continue;
        const struct if_data *d = i->ifa_data;
        *rx += d->ifi_ibytes;
        *tx += d->ifi_obytes;
    }
    freeifaddrs(ifa);
    return 0;
}
#else
static int read_counters(unsigned long long *rx, unsigned long long *tx)
{
    char line[512];
    FILE *f = fopen("/proc/net/dev", "r");
    if (!f)
        return -1;
    *rx = *tx = 0;
    while (fgets(line, sizeof line, f)) {
        char *colon = strchr(line, ':');
        if (!colon)
            continue; /* header lines */
        *colon = '\0';
        char *name = line;
        while (*name == ' ')
            name++;
        if (strcmp(name, "lo") == 0)
            continue;
        if (!wanted(name))
            continue;
        if (!only[0] && virtual_if(name))
            continue;
        unsigned long long r, t, skip[7];
        if (sscanf(colon + 1, "%llu %llu %llu %llu %llu %llu %llu %llu %llu",
                   &r, &skip[0], &skip[1], &skip[2], &skip[3], &skip[4], &skip[5],
                   &skip[6], &t) == 9) {
            *rx += r;
            *tx += t;
        }
    }
    fclose(f);
    return 0;
}
#endif

static int net_init(const sysmon_host_t *host)
{
    H = host;
    const char *v = H->abi >= 2 ? H->option("net", "interfaces") : NULL;
    if (v)
        snprintf(only, sizeof only, "%s", v);
    if (read_counters(&prev_rx, &prev_tx) < 0) {
        H->log(SYSMON_LOG_WARN, "net: interface counters not available");
        return -1;
    }
    prev_t = now();
    H->set_help("net_rx_bytes_per_second", "Received bytes per second, all interfaces but loopback.");
    H->set_help("net_tx_bytes_per_second", "Transmitted bytes per second, all interfaces but loopback.");
    return 0;
}

static int net_collect(void)
{
    unsigned long long rx, tx;
    if (read_counters(&rx, &tx) < 0)
        return -1;
    double t = now(), dt = t - prev_t;
    /* macOS if_data counters are 32 bits and wrap at 4 GiB; interfaces
     * also come and go. Skip any interval where a total went down. */
    if (dt > 0 && rx >= prev_rx && tx >= prev_tx) {
        H->set("net_rx_bytes_per_second", (double)(rx - prev_rx) / dt);
        H->set("net_tx_bytes_per_second", (double)(tx - prev_tx) / dt);
    }
    prev_rx = rx;
    prev_tx = tx;
    prev_t = t;
    return 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "net", "1.1", net_init, net_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
