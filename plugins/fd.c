/*
 * fd plugin: system-wide open file handles.
 */
#include <stdio.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

#include "sysmon/plugin.h"

static const sysmon_host_t *H;

#if defined(__APPLE__) || defined(__FreeBSD__)
#ifdef __APPLE__
#define OPEN_FILES "kern.num_files"
#else
#define OPEN_FILES "kern.openfiles"
#endif
static int read_fds(double *used, double *max)
{
    int n, m;
    size_t len = sizeof n;

    if (sysctlbyname(OPEN_FILES, &n, &len, NULL, 0) < 0)
        return -1;
    len = sizeof m;
    if (sysctlbyname("kern.maxfiles", &m, &len, NULL, 0) < 0)
        return -1;
    *used = n;
    *max = m;
    return 0;
}
#else
/* /proc/sys/fs/file-nr: allocated, allocated but unused (always 0 since 2.6), max */
static int read_fds(double *used, double *max)
{
    unsigned long long alloc, unused, m;
    FILE *f = fopen("/proc/sys/fs/file-nr", "r");

    if (!f)
        return -1;
    int n = fscanf(f, "%llu %llu %llu", &alloc, &unused, &m);
    fclose(f);
    if (n != 3)
        return -1;
    *used = (double)(alloc - unused);
    *max = (double)m;
    return 0;
}
#endif

static int fd_init(const sysmon_host_t *host)
{
    double used, max;

    H = host;
    if (read_fds(&used, &max) < 0) {
        H->log(SYSMON_LOG_WARN, "fd: cannot read the file handle counters");
        return -1;
    }
    H->set_help("fd_used", "Open file handles, system wide.");
    H->set_help("fd_max", "Maximum number of file handles.");
    H->set_help("fd_used_percent", "fd_used / fd_max, in percent.");
    return 0;
}

static int fd_collect(void)
{
    double used, max;

    if (read_fds(&used, &max) < 0)
        return -1;
    H->set("fd_used", used);
    H->set("fd_max", max);
    if (max > 0)
        H->set("fd_used_percent", 100.0 * used / max);
    return 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "fd", "1.0", fd_init, fd_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
