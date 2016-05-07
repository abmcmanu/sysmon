/*
 * Linux collector: everything comes from /proc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "collector.h"
#include "log.h"
#include "metrics.h"

static unsigned long long prev_idle, prev_total;
static int have_prev;

static int read_cpu(unsigned long long *idle, unsigned long long *total)
{
    unsigned long long user = 0, nice = 0, sys = 0, idl = 0, iowait = 0;
    unsigned long long irq = 0, softirq = 0, steal = 0;

    FILE *f = fopen("/proc/stat", "r");
    if (!f)
        return -1;
    int n = fscanf(f, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
                   &user, &nice, &sys, &idl, &iowait, &irq, &softirq, &steal);
    fclose(f);
    if (n < 4)
        return -1;

    *idle = idl + iowait;
    *total = user + nice + sys + idl + iowait + irq + softirq + steal;
    return 0;
}

static void sample_cpu(void)
{
    unsigned long long idle, total;

    if (read_cpu(&idle, &total) < 0) {
        log_warn("cpu: cannot read /proc/stat");
        return;
    }
    /* Counters can go backwards after a VM migration or a hotplug:
     * drop that interval instead of reporting garbage. */
    if (have_prev && total > prev_total && idle >= prev_idle) {
        double dt = (double)(total - prev_total);
        double di = (double)(idle - prev_idle);
        metrics_set("cpu_usage_percent", 100.0 * (dt - di) / dt);
    }
    prev_idle = idle;
    prev_total = total;
    have_prev = 1;
}

static void sample_mem(void)
{
    unsigned long long total = 0, avail = 0, free_ = 0, buffers = 0, cached = 0;
    char line[256], key[64];
    unsigned long long val;

    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) {
        log_warn("mem: cannot read /proc/meminfo");
        return;
    }
    while (fgets(line, sizeof line, f)) {
        if (sscanf(line, "%63[^:]: %llu", key, &val) != 2)
            continue;
        if (strcmp(key, "MemTotal") == 0)
            total = val;
        else if (strcmp(key, "MemAvailable") == 0)
            avail = val;
        else if (strcmp(key, "MemFree") == 0)
            free_ = val;
        else if (strcmp(key, "Buffers") == 0)
            buffers = val;
        else if (strcmp(key, "Cached") == 0)
            cached = val;
    }
    fclose(f);

    if (total == 0)
        return;
    if (avail == 0) /* kernel < 3.14 */
        avail = free_ + buffers + cached;

    double used = (double)(total - avail) * 1024.0;
    metrics_set("mem_total_bytes", (double)total * 1024.0);
    metrics_set("mem_used_bytes", used);
    metrics_set("mem_used_percent", 100.0 * used / ((double)total * 1024.0));
}

static void sample_load(void)
{
    double l[3];

    FILE *f = fopen("/proc/loadavg", "r");
    if (!f) {
        log_warn("load: cannot read /proc/loadavg");
        return;
    }
    int n = fscanf(f, "%lf %lf %lf", &l[0], &l[1], &l[2]);
    fclose(f);
    if (n != 3)
        return;
    metrics_set("load1", l[0]);
    metrics_set("load5", l[1]);
    metrics_set("load15", l[2]);
}

int collect_init(void)
{
    metrics_set_help("cpu_usage_percent", "CPU busy time over the last interval, in percent.");
    metrics_set_help("mem_total_bytes", "Physical memory.");
    metrics_set_help("mem_used_bytes", "Memory in use (total - available).");
    metrics_set_help("mem_used_percent", "Memory in use, in percent.");
    metrics_set_help("load1", "Load average over 1 minute.");
    metrics_set_help("load5", "Load average over 5 minutes.");
    metrics_set_help("load15", "Load average over 15 minutes.");

    /* prime the cpu counters so the first real tick has a delta */
    unsigned long long idle, total;
    if (read_cpu(&idle, &total) < 0) {
        log_err("cannot read /proc/stat: is /proc mounted?");
        return -1;
    }
    prev_idle = idle;
    prev_total = total;
    have_prev = 1;
    return 0;
}

int collect_sample(void)
{
    sample_cpu();
    sample_mem();
    sample_load();
    return 0;
}
