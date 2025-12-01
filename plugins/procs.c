/*
 * procs plugin: process and thread counts.
 */
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "sysmon/plugin.h"

static const sysmon_host_t *H;

static int count_procs(void)
{
    DIR *d = opendir("/proc");
    struct dirent *e;
    int n = 0;

    if (!d)
        return -1;
    while ((e = readdir(d)))
        if (isdigit((unsigned char)e->d_name[0]))
            n++;
    closedir(d);
    return n;
}

static int procs_init(const sysmon_host_t *host)
{
    H = host;
    if (count_procs() < 0) {
        H->log(SYSMON_LOG_WARN, "procs: cannot list processes");
        return -1;
    }
    return 0;
}

static int procs_collect(void)
{
    int n = count_procs();
    if (n < 0)
        return -1;
    H->set("procs_total", n);
    return 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "procs", "1.0", procs_init, procs_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
