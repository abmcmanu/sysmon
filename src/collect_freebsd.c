/*
 * FreeBSD collector: sysctl only, no procfs needed.
 */
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "collector.h"
#include "log.h"
#include "metrics.h"

static unsigned long long prev_idle, prev_total;
static int have_prev;

/* vm.stats counters are 32 or 64 bits depending on the release */
static int sysctl_num(const char *name, unsigned long long *out)
{
    union {
        uint32_t u32;
        uint64_t u64;
    } v;
    size_t len = sizeof v;

    memset(&v, 0, sizeof v);
    if (sysctlbyname(name, &v, &len, NULL, 0) < 0)
        return -1;
    *out = len == sizeof v.u32 ? v.u32 : v.u64;
    return 0;
}

static int read_cpu(unsigned long long *idle, unsigned long long *total)
{
    long cp[CPUSTATES];
    size_t len = sizeof cp;

    if (sysctlbyname("kern.cp_time", cp, &len, NULL, 0) < 0)
        return -1;
    *total = 0;
    for (int i = 0; i < CPUSTATES; i++)
        *total += (unsigned long long)cp[i];
    *idle = (unsigned long long)cp[CP_IDLE];
    return 0;
}

static void sample_cpu(void)
{
    unsigned long long idle, total;

    if (read_cpu(&idle, &total) < 0) {
        log_warn("cpu: kern.cp_time failed");
        return;
    }
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
    unsigned long long physmem, pagesize, free_, inactive, laundry = 0;

    if (sysctl_num("hw.physmem", &physmem) < 0 || sysctl_num("hw.pagesize", &pagesize) < 0 ||
        sysctl_num("vm.stats.vm.v_free_count", &free_) < 0 ||
        sysctl_num("vm.stats.vm.v_inactive_count", &inactive) < 0)
        return;
    sysctl_num("vm.stats.vm.v_laundry_count", &laundry); /* FreeBSD 12+ */

    double avail = (double)(free_ + inactive + laundry) * (double)pagesize;
    double used = (double)physmem - avail;
    metrics_set("mem_total_bytes", (double)physmem);
    metrics_set("mem_used_bytes", used);
    metrics_set("mem_used_percent", 100.0 * used / (double)physmem);
}

static void sample_load(void)
{
    double l[3];

    if (getloadavg(l, 3) != 3)
        return;
    metrics_set("load1", l[0]);
    metrics_set("load5", l[1]);
    metrics_set("load15", l[2]);
}

int collect_init(void)
{
    unsigned long long idle, total;

    metrics_set_help("cpu_usage_percent", "CPU busy time over the last interval, in percent.");
    metrics_set_help("mem_total_bytes", "Physical memory.");
    metrics_set_help("mem_used_bytes", "Memory in use (total - free - inactive - laundry).");
    metrics_set_help("mem_used_percent", "Memory in use, in percent.");
    metrics_set_help("load1", "Load average over 1 minute.");
    metrics_set_help("load5", "Load average over 5 minutes.");
    metrics_set_help("load15", "Load average over 15 minutes.");
    if (read_cpu(&idle, &total) < 0) {
        log_err("kern.cp_time not available");
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
