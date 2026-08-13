/*
 * procs plugin: process and thread counts.
 */
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#ifdef __FreeBSD__
#include <sys/user.h>
#endif
#else
#include <ctype.h>
#include <dirent.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "sysmon/plugin.h"

static const sysmon_host_t *H;
static unsigned long long prev_forks;
static double prev_t;

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

#if defined(__APPLE__) || defined(__FreeBSD__)
#ifdef __APPLE__
#define PROC_WHAT KERN_PROC_ALL
#else
#define PROC_WHAT KERN_PROC_PROC /* processes, not threads */
#endif
static int count_procs(void)
{
    int mib[3] = { CTL_KERN, KERN_PROC, PROC_WHAT };
    size_t len = 0;

    if (sysctl(mib, 3, NULL, &len, NULL, 0) < 0)
        return -1;
    return (int)(len / sizeof(struct kinfo_proc));
}
#else
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

/* the state is the field right after "(comm)", and comm may contain ") " */
static int count_zombies(void)
{
    DIR *d = opendir("/proc");
    struct dirent *e;
    char path[64], line[512];
    int z = 0;

    if (!d)
        return -1;
    while ((e = readdir(d))) {
        if (!isdigit((unsigned char)e->d_name[0]))
            continue;
        snprintf(path, sizeof path, "/proc/%.20s/stat", e->d_name);
        FILE *f = fopen(path, "r");
        if (!f)
            continue; /* exited meanwhile */
        if (fgets(line, sizeof line, f)) {
            char *p = strrchr(line, ')');
            if (p && p[1] == ' ' && p[2] == 'Z')
                z++;
        }
        fclose(f);
    }
    closedir(d);
    return z;
}

/* procs_running / procs_blocked / processes (forks since boot) */
static int read_stat(unsigned long long *running, unsigned long long *blocked,
                     unsigned long long *forks)
{
    char line[256];
    FILE *f = fopen("/proc/stat", "r");

    if (!f)
        return -1;
    while (fgets(line, sizeof line, f)) {
        sscanf(line, "procs_running %llu", running);
        sscanf(line, "procs_blocked %llu", blocked);
        sscanf(line, "processes %llu", forks);
    }
    fclose(f);
    return 0;
}

/* 4th field of /proc/loadavg: runnable/total scheduling entities */
static int read_threads(unsigned long long *threads)
{
    double l1, l5, l15;
    unsigned long long runnable;
    FILE *f = fopen("/proc/loadavg", "r");

    if (!f)
        return -1;
    int n = fscanf(f, "%lf %lf %lf %llu/%llu", &l1, &l5, &l15, &runnable, threads);
    fclose(f);
    return n == 5 ? 0 : -1;
}
#endif

static int procs_init(const sysmon_host_t *host)
{
    H = host;
    if (count_procs() < 0) {
        H->log(SYSMON_LOG_WARN, "procs: cannot list processes");
        return -1;
    }
    H->set_help("procs_total", "Number of processes.");
#if !defined(__APPLE__) && !defined(__FreeBSD__)
    H->set_help("procs_running", "Processes in the run queue.");
    H->set_help("procs_blocked", "Processes blocked on I/O.");
    H->set_help("threads_total", "Number of threads.");
    H->set_help("forks_per_second", "Processes created per second.");
#endif
    prev_forks = 0;
    prev_t = now();
    return 0;
}

static int procs_collect(void)
{
    int n = count_procs();
    if (n < 0)
        return -1;
    H->set("procs_total", n);
#if !defined(__APPLE__) && !defined(__FreeBSD__)

    unsigned long long running = 0, blocked = 0, forks = 0;
    if (read_stat(&running, &blocked, &forks) == 0) {
        H->set("procs_running", (double)running);
        H->set("procs_blocked", (double)blocked);
        double t = now();
        if (prev_forks && forks >= prev_forks && t > prev_t)
            H->set("forks_per_second", (double)(forks - prev_forks) / (t - prev_t));
        prev_forks = forks;
        prev_t = t;
    }

    int z = count_zombies();
    if (z >= 0)
        H->set("procs_zombie", z);

    unsigned long long threads;
    if (read_threads(&threads) == 0)
        H->set("threads_total", (double)threads);
#endif
    return 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "procs", "1.0", procs_init, procs_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
