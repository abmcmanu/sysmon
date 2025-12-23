/*
 * diskio plugin: block device throughput from /proc/diskstats (Linux).
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "sysmon/plugin.h"

#define MAX_DEVS 64

typedef struct disk {
    char name[32];
    unsigned long long rd_ios, rd_sectors, wr_ios, wr_sectors, io_ms;
} disk_t;

static const sysmon_host_t *H;
static disk_t devs[MAX_DEVS];
static int ndevs;
static double prev_t;

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static int ignored(const char *name)
{
    static const char *const skip[] = { "loop", "ram", "zram", "fd", "sr" };

    for (size_t i = 0; i < sizeof skip / sizeof skip[0]; i++)
        if (strncmp(name, skip[i], strlen(skip[i])) == 0)
            return 1;
    /* partitions have no /sys/block entry; '/' in names becomes '!' there */
    char path[96];
    snprintf(path, sizeof path, "/sys/block/%s", name);
    for (char *p = path + strlen("/sys/block/"); *p; p++)
        if (*p == '/')
            *p = '!';
    return access(path, F_OK) != 0;
}

static int read_stats(disk_t *out, int max)
{
    char line[512];
    int n = 0;

    FILE *f = fopen("/proc/diskstats", "r");
    if (!f)
        return -1;
    while (n < max && fgets(line, sizeof line, f)) {
        disk_t d;
        unsigned major, minor;
        unsigned long long rd_merged, rd_ms, wr_merged, wr_ms, inflight;

        memset(&d, 0, sizeof d);
        if (sscanf(line, "%u %u %31s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", &major,
                   &minor, d.name, &d.rd_ios, &rd_merged, &d.rd_sectors, &rd_ms, &d.wr_ios,
                   &wr_merged, &d.wr_sectors, &wr_ms, &inflight, &d.io_ms) != 13)
            continue;
        if (ignored(d.name))
            continue;
        out[n++] = d;
    }
    fclose(f);
    return n;
}

static const disk_t *find(const char *name)
{
    for (int i = 0; i < ndevs; i++)
        if (strcmp(devs[i].name, name) == 0)
            return &devs[i];
    return NULL;
}

static void metric(const char *dev, const char *what, double v)
{
    char name[96];
    if (snprintf(name, sizeof name, "disk_%s_%s", dev, what) >= (int)sizeof name)
        return;
    H->set(name, v);
}

static int diskio_init(const sysmon_host_t *host)
{
    H = host;
    ndevs = read_stats(devs, MAX_DEVS);
    if (ndevs < 0) {
        H->log(SYSMON_LOG_WARN, "diskio: /proc/diskstats not available");
        return -1;
    }
    prev_t = now();
    return 0;
}

static int diskio_collect(void)
{
    disk_t cur[MAX_DEVS];
    int n = read_stats(cur, MAX_DEVS);

    if (n < 0)
        return -1;
    H->set("disk_devices", n);

    double t = now(), dt = t - prev_t;
    for (int i = 0; i < n; i++) {
        const disk_t *c = &cur[i], *p = find(c->name);
        if (!p || dt <= 0 || c->rd_sectors < p->rd_sectors || c->wr_sectors < p->wr_sectors)
            continue;
        /* diskstats counts 512-byte sectors whatever the device's sector size */
        double rd = (double)(c->rd_sectors - p->rd_sectors) * 512.0 / dt;
        double wr = (double)(c->wr_sectors - p->wr_sectors) * 512.0 / dt;
        metric(c->name, "read_bytes_per_second", rd);
        metric(c->name, "write_bytes_per_second", wr);
        metric(c->name, "iops", (double)((c->rd_ios - p->rd_ios) + (c->wr_ios - p->wr_ios)) / dt);
        double busy = (double)(c->io_ms - p->io_ms) / (dt * 10.0);
        metric(c->name, "busy_percent", busy > 100.0 ? 100.0 : busy);
    }
    prev_t = t;
    memcpy(devs, cur, sizeof(disk_t) * (size_t)n);
    ndevs = n;
    return 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "diskio", "1.0", diskio_init, diskio_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
