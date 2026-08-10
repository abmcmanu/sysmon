/*
 * thermal plugin: temperatures from /sys/class/thermal (Linux).
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "sysmon/plugin.h"

#define MAX_ZONES 32

static const sysmon_host_t *H;

static int read_zone(int i, char *type, size_t size, double *celsius)
{
    char path[96];
    long milli;
    FILE *f;

    snprintf(path, sizeof path, "/sys/class/thermal/thermal_zone%d/temp", i);
    if (!(f = fopen(path, "r")))
        return -1;
    int n = fscanf(f, "%ld", &milli);
    fclose(f);
    if (n != 1)
        return -1;
    *celsius = milli / 1000.0;

    snprintf(path, sizeof path, "/sys/class/thermal/thermal_zone%d/type", i);
    if (!(f = fopen(path, "r")) || !fgets(type, (int)size, f))
        snprintf(type, size, "zone%d", i);
    if (f)
        fclose(f);
    type[strcspn(type, "\n")] = '\0';
    return 0;
}

static int read_hwmon(int h, int t, char *type, size_t size, double *celsius)
{
    char path[96], chip[24] = "hwmon";
    long milli;
    FILE *f;

    snprintf(path, sizeof path, "/sys/class/hwmon/hwmon%d/temp%d_input", h, t);
    if (!(f = fopen(path, "r")))
        return -1;
    int n = fscanf(f, "%ld", &milli);
    fclose(f);
    if (n != 1)
        return -1;
    *celsius = milli / 1000.0;

    snprintf(path, sizeof path, "/sys/class/hwmon/hwmon%d/name", h);
    if ((f = fopen(path, "r"))) {
        if (fgets(chip, sizeof chip, f))
            chip[strcspn(chip, "\n")] = '\0';
        fclose(f);
    }
    snprintf(type, size, "hwmon%d_%s_%d", h, chip, t);
    return 0;
}

static void clean_name(char *s)
{
    for (; *s; s++)
        *s = isalnum((unsigned char)*s) ? (char)tolower((unsigned char)*s) : '_';
}

static int scan(int set)
{
    char type[64];
    char name[96];
    char seen[MAX_ZONES][64];
    double c, max = -1000;
    int zones = 0;

    for (int i = 0; i < MAX_ZONES; i++) {
        if (read_zone(i, type, sizeof type, &c) < 0)
            continue;
        if (c <= -40 || c >= 200) /* disconnected sensors report garbage */
            continue;
        if (c > max)
            max = c;
        clean_name(type);
        int dup = 0;
        for (int j = 0; j < zones; j++)
            if (strcmp(seen[j], type) == 0)
                dup++;
        snprintf(seen[zones], sizeof seen[0], "%s", type);
        if (dup)
            snprintf(name, sizeof name, "temp_%s_%d_celsius", type, dup);
        else
            snprintf(name, sizeof name, "temp_%s_celsius", type);
        if (set)
            H->set(name, c);
        zones++;
    }
    /* no thermal zones, as on many x86 desktops: try hwmon sensors */
    for (int h = 0, none = zones == 0; none && h < 16; h++) {
        for (int t = 1; t <= 8; t++) {
            if (read_hwmon(h, t, type, sizeof type, &c) < 0)
                continue;
            if (c <= -40 || c >= 200)
                continue;
            if (c > max)
                max = c;
            clean_name(type);
            snprintf(name, sizeof name, "temp_%s_celsius", type);
            if (set)
                H->set(name, c);
            zones++;
        }
    }
    if (zones && set)
        H->set("temp_max_celsius", max);
    return zones;
}

static int thermal_init(const sysmon_host_t *host)
{
    H = host;
    if (scan(0) == 0) {
        H->log(SYSMON_LOG_WARN, "thermal: no thermal zones or hwmon sensors");
        return -1;
    }
    H->set_help("temp_max_celsius", "Hottest thermal zone, degrees Celsius.");
    return 0;
}

static int thermal_collect(void)
{
    return scan(1) ? 0 : -1;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "thermal", "1.0", thermal_init, thermal_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
