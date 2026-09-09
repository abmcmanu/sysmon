/*
 * battery plugin: laptops and single-board computers on battery (Linux).
 */
#include <stdio.h>
#include <string.h>

#include "sysmon/plugin.h"

static const sysmon_host_t *H;
static char dir[64];

static double strtod_safe(const char *s)
{
    double v = 0;
    sscanf(s, "%lf", &v);
    return v;
}

static int read_line(const char *file, char *buf, size_t size)
{
    char path[128];
    FILE *f;

    snprintf(path, sizeof path, "%s/%s", dir, file);
    if (!(f = fopen(path, "r")))
        return -1;
    char *ok = fgets(buf, (int)size, f);
    fclose(f);
    if (!ok)
        return -1;
    buf[strcspn(buf, "\n")] = '\0';
    return 0;
}

static int battery_init(const sysmon_host_t *host)
{
    char buf[32];

    H = host;
    for (int i = 0; i < 4; i++) {
        snprintf(dir, sizeof dir, "/sys/class/power_supply/BAT%d", i);
        if (read_line("capacity", buf, sizeof buf) == 0) {
            return 0;
        }
    }
    H->log(SYSMON_LOG_INFO, "battery: no battery found");
    return -1;
}

static int battery_collect(void)
{
    char buf[32];

    if (read_line("capacity", buf, sizeof buf) < 0)
        return -1;
    H->set("battery_percent", strtod_safe(buf));
    return 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "battery", "1.0", battery_init, battery_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
