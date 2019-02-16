/*
 * net plugin: receive/transmit throughput summed over all
 * non-loopback interfaces.
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "sysmon/plugin.h"

static const sysmon_host_t *H;
static unsigned long long prev_rx, prev_tx;
static double prev_t;

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

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

static int net_init(const sysmon_host_t *host)
{
    H = host;
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
    if (dt > 0) {
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
