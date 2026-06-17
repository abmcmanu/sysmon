/*
 * uptime plugin: seconds since boot.
 */
#include <stdio.h>
#include <time.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/sysctl.h>
#include <sys/time.h>
#endif

#include "sysmon/plugin.h"

static const sysmon_host_t *H;

static int read_uptime(double *secs)
{
#if defined(__APPLE__) || defined(__FreeBSD__)
    struct timeval boot;
    size_t len = sizeof boot;
    int mib[2] = { CTL_KERN, KERN_BOOTTIME };
    if (sysctl(mib, 2, &boot, &len, NULL, 0) < 0 || boot.tv_sec == 0)
        return -1;
    *secs = difftime(time(NULL), boot.tv_sec);
    return 0;
#else
    FILE *f = fopen("/proc/uptime", "r");
    if (!f)
        return -1;
    int n = fscanf(f, "%lf", secs);
    fclose(f);
    return n == 1 ? 0 : -1;
#endif
}

static int uptime_init(const sysmon_host_t *host)
{
    double s;
    H = host;
    if (read_uptime(&s) < 0) {
        H->log(SYSMON_LOG_WARN, "uptime: cannot read system uptime");
        return -1;
    }
    H->set_help("uptime_seconds", "Seconds since boot.");
    return 0;
}

static int uptime_collect(void)
{
    double s;
    if (read_uptime(&s) < 0)
        return -1;
    H->set("uptime_seconds", s);
    return 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "uptime", "1.0", uptime_init, uptime_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
