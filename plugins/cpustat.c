/*
 * cpustat plugin: per-core usage and where cpu time goes.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef __APPLE__
#include <mach/mach.h>
#endif

#include "sysmon/plugin.h"

#define MAX_CPUS 256

typedef struct times {
    unsigned long long user, nice, sys, idle, iowait, irq, softirq, steal;
} times_t;

static const sysmon_host_t *H;
static times_t prev[MAX_CPUS + 1]; /* [0] is the aggregate */
static int ncpu;
static int per_core = 1;
static unsigned long long cur_ctxt, cur_intr, prev_ctxt, prev_intr;
static double prev_t;

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static unsigned long long busy(const times_t *t)
{
    return t->user + t->nice + t->sys + t->irq + t->softirq + t->steal;
}

static unsigned long long total(const times_t *t)
{
    return busy(t) + t->idle + t->iowait;
}

static double pct(unsigned long long part, unsigned long long whole)
{
    return whole ? 100.0 * (double)part / (double)whole : 0.0;
}

#ifdef __APPLE__
static int read_times(times_t *cur)
{
    processor_info_array_t info;
    mach_msg_type_number_t ninfo;
    natural_t n;

    if (host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &n, &info, &ninfo) !=
        KERN_SUCCESS)
        return -1;
    processor_cpu_load_info_t load = (processor_cpu_load_info_t)info;
    if (n > MAX_CPUS)
        n = MAX_CPUS;
    memset(cur, 0, sizeof(times_t) * (n + 1));
    for (natural_t i = 0; i < n; i++) {
        times_t *t = &cur[i + 1];
        t->user = load[i].cpu_ticks[CPU_STATE_USER];
        t->nice = load[i].cpu_ticks[CPU_STATE_NICE];
        t->sys = load[i].cpu_ticks[CPU_STATE_SYSTEM];
        t->idle = load[i].cpu_ticks[CPU_STATE_IDLE];
        cur[0].user += t->user;
        cur[0].nice += t->nice;
        cur[0].sys += t->sys;
        cur[0].idle += t->idle;
    }
    vm_deallocate(mach_task_self(), (vm_address_t)info, ninfo * sizeof(integer_t));
    return (int)n;
}
#else
static int read_times(times_t *cur)
{
    char line[1024], name[16];
    int n = 0;

    FILE *f = fopen("/proc/stat", "r");
    if (!f)
        return -1;
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, "cpu", 3) != 0) {
            unsigned long long v;
            if (sscanf(line, "ctxt %llu", &v) == 1)
                cur_ctxt = v;
            else if (sscanf(line, "intr %llu", &v) == 1)
                cur_intr = v;
            continue;
        }
        times_t t;
        memset(&t, 0, sizeof t);
        if (sscanf(line, "%15s %llu %llu %llu %llu %llu %llu %llu %llu", name, &t.user, &t.nice,
                   &t.sys, &t.idle, &t.iowait, &t.irq, &t.softirq, &t.steal) < 5)
            continue;
        int idx = 0;
        if (name[3] != '\0') {
            idx = atoi(name + 3) + 1;
            if (idx > MAX_CPUS)
                continue;
            if (idx > n)
                n = idx;
        }
        cur[idx] = t;
    }
    fclose(f);
    return n;
}
#endif

static int cpustat_init(const sysmon_host_t *host)
{
    H = host;
    const char *v = H->abi >= 2 ? H->option("cpustat", "per_core") : NULL;
    if (v && (strcmp(v, "no") == 0 || strcmp(v, "false") == 0 || strcmp(v, "0") == 0))
        per_core = 0;
    ncpu = read_times(prev);
    if (ncpu <= 0) {
        H->log(SYSMON_LOG_WARN, "cpustat: cannot read cpu times");
        return -1;
    }
    prev_ctxt = cur_ctxt;
    prev_intr = cur_intr;
    prev_t = now();
    H->set_help("cpu_count", "Number of online cpus.");
    H->set_help("cpu_user_percent", "Time spent in user space, including niced processes.");
    H->set_help("cpu_system_percent", "Time spent in the kernel, including interrupts.");
    H->set_help("cpu_iowait_percent", "Idle time with I/O outstanding.");
    H->set_help("cpu_steal_percent", "Time stolen by the hypervisor.");
    H->set_help("context_switches_per_second", "Context switches per second.");
    H->set_help("interrupts_per_second", "Interrupts per second.");
    H->set("cpu_count", ncpu);
    return 0;
}

static int cpustat_collect(void)
{
    times_t cur[MAX_CPUS + 1];
    int n = read_times(cur);

    if (n <= 0)
        return -1;
    if (n != ncpu) {
        H->log(SYSMON_LOG_INFO, "cpustat: %d cpus online, was %d", n, ncpu);
        ncpu = n;
        memcpy(prev, cur, sizeof(times_t) * (size_t)(n + 1));
        H->set("cpu_count", n);
        return 0;
    }

    char name[64];
    for (int i = 1; per_core && i <= n; i++) {
        if (total(&cur[i]) <= total(&prev[i]))
            continue;
        snprintf(name, sizeof name, "cpu%d_usage_percent", i - 1);
        H->set(name, pct(busy(&cur[i]) - busy(&prev[i]), total(&cur[i]) - total(&prev[i])));
    }

    const times_t *c = &cur[0], *p = &prev[0];
    if (total(c) > total(p)) {
        unsigned long long dt = total(c) - total(p);
        H->set("cpu_user_percent", pct((c->user + c->nice) - (p->user + p->nice), dt));
        H->set("cpu_system_percent",
               pct((c->sys + c->irq + c->softirq) - (p->sys + p->irq + p->softirq), dt));
        H->set("cpu_iowait_percent", pct(c->iowait - p->iowait, dt));
        H->set("cpu_steal_percent", pct(c->steal - p->steal, dt));
    }

    double t = now(), elapsed = t - prev_t;
    if (cur_ctxt && elapsed > 0 && cur_ctxt >= prev_ctxt && cur_intr >= prev_intr) {
        H->set("context_switches_per_second", (double)(cur_ctxt - prev_ctxt) / elapsed);
        H->set("interrupts_per_second", (double)(cur_intr - prev_intr) / elapsed);
    }
    prev_ctxt = cur_ctxt;
    prev_intr = cur_intr;
    prev_t = t;
    memcpy(prev, cur, sizeof(times_t) * (size_t)(n + 1));
    return 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "cpustat", "1.0", cpustat_init, cpustat_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
