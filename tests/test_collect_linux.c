/* the Linux collector against fake /proc files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "collector.h"
#include "log.h"
#include "metrics.h"
#include "test.h"

#ifdef __linux__
static char root[] = "/tmp/sysmon-proc-XXXXXX";

static void put(const char *name, const char *text)
{
    char path[128];
    snprintf(path, sizeof path, "%s/%s", root, name);
    FILE *f = fopen(path, "w");
    fputs(text, f);
    fclose(f);
}

static double get(const char *name)
{
    const metric_t *m = metrics_get(name);
    return m ? m->value : NAN;
}
#endif

int main(void)
{
#ifdef __linux__
    log_set_level(-1);
    if (!mkdtemp(root))
        return 1;
    setenv("SYSMON_PROC_ROOT", root, 1);

    /* user nice system idle iowait irq softirq steal */
    put("stat", "cpu  100 0 100 800 0 0 0 0\n");
    put("meminfo", "MemTotal: 1000 kB\nMemAvailable: 250 kB\nSwapTotal: 0 kB\nSwapFree: 0 kB\n");
    CHECK(collect_init() == 0);
    put("stat", "cpu  150 0 150 900 0 0 0 0\n");
    collect_sample();
    CHECK_NEAR(get("cpu_usage_percent"), 50.0, 1e-9);
    CHECK_NEAR(get("mem_used_percent"), 75.0, 1e-9);
    CHECK_NEAR(get("mem_total_bytes"), 1024000.0, 1e-9);

    /* iowait counts as idle */
    put("stat", "cpu  150 0 150 900 100 0 0 0\n");
    collect_sample();
    CHECK_NEAR(get("cpu_usage_percent"), 0.0, 1e-9);

    /* counters going backwards (VM migration) leave the previous value */
    put("stat", "cpu  10 0 10 10 0 0 0 0\n");
    collect_sample();
    CHECK_NEAR(get("cpu_usage_percent"), 0.0, 1e-9);

    /* kernels before 3.14 have no MemAvailable */
    put("meminfo", "MemTotal: 1000 kB\nMemFree: 100 kB\nBuffers: 50 kB\nCached: 50 kB\n"
                   "SReclaimable: 100 kB\n");
    collect_sample();
    CHECK_NEAR(get("mem_used_percent"), 70.0, 1e-9);

    char cmd[64];
    snprintf(cmd, sizeof cmd, "rm -rf %s", root);
    CHECK(system(cmd) == 0);
#endif
    return test_report("collect");
}
