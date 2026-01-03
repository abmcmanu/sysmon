/*
 * psi plugin: Linux pressure stall information (4.20+, CONFIG_PSI).
 *
 * "some" is the share of time at least one task was stalled on the
 * resource, "full" the share of time all non-idle tasks were.
 */
#include <stdio.h>
#include <string.h>

#include "sysmon/plugin.h"

static const sysmon_host_t *H;
static const char *const resources[] = { "cpu", "memory", "io" };

static int read_pressure(const char *res)
{
    char path[64], line[256], kind[8];
    double avg10, avg60, avg300;
    int found = 0;

    snprintf(path, sizeof path, "/proc/pressure/%s", res);
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;
    while (fgets(line, sizeof line, f)) {
        if (sscanf(line, "%7s avg10=%lf avg60=%lf avg300=%lf", kind, &avg10, &avg60, &avg300) != 4)
            continue;
        char name[64];
        snprintf(name, sizeof name, "psi_%s_%s_avg10", res, kind);
        H->set(name, avg10);
        snprintf(name, sizeof name, "psi_%s_%s_avg60", res, kind);
        H->set(name, avg60);
        found++;
    }
    fclose(f);
    return found ? 0 : -1;
}

static int psi_init(const sysmon_host_t *host)
{
    H = host;
    if (read_pressure("cpu") < 0) {
        H->log(SYSMON_LOG_WARN, "psi: /proc/pressure not available");
        return -1;
    }
    H->set_help("psi_cpu_some_avg10", "Share of time some tasks waited for cpu, last 10 s, percent.");
    H->set_help("psi_memory_some_avg10", "Share of time some tasks waited for memory, last 10 s, percent.");
    H->set_help("psi_memory_full_avg10", "Share of time all tasks waited for memory, last 10 s, percent.");
    H->set_help("psi_io_some_avg10", "Share of time some tasks waited for I/O, last 10 s, percent.");
    H->set_help("psi_io_full_avg10", "Share of time all tasks waited for I/O, last 10 s, percent.");
    return 0;
}

static int psi_collect(void)
{
    int errors = 0;
    for (size_t i = 0; i < sizeof resources / sizeof resources[0]; i++)
        if (read_pressure(resources[i]) < 0)
            errors++;
    return errors == (int)(sizeof resources / sizeof resources[0]) ? -1 : 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "psi", "1.0", psi_init, psi_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
