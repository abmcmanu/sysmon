/*
 * macOS collector: Mach host statistics + sysctl.
 *
 * There is no /proc on Darwin. CPU ticks come from host_statistics(),
 * memory from host_statistics64() and the "used" figure follows
 * Activity Monitor: app memory + wired + compressed.
 */
#include <mach/mach.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/sysctl.h>
#include <sys/types.h>

#include "collector.h"
#include "log.h"
#include "metrics.h"

static mach_port_t host;
static unsigned long long prev_idle, prev_total;
static int have_prev;

static int read_cpu(unsigned long long *idle, unsigned long long *total)
{
    host_cpu_load_info_data_t info;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;

    if (host_statistics(host, HOST_CPU_LOAD_INFO, (host_info_t)&info, &count) != KERN_SUCCESS)
        return -1;
    unsigned long long user = info.cpu_ticks[CPU_STATE_USER];
    unsigned long long sys = info.cpu_ticks[CPU_STATE_SYSTEM];
    unsigned long long nice = info.cpu_ticks[CPU_STATE_NICE];
    unsigned long long idl = info.cpu_ticks[CPU_STATE_IDLE];
    *idle = idl;
    *total = user + sys + nice + idl;
    return 0;
}

static void sample_cpu(void)
{
    unsigned long long idle, total;

    if (read_cpu(&idle, &total) < 0) {
        log_warn("cpu: host_statistics failed");
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
    uint64_t memsize = 0;
    size_t len = sizeof memsize;
    vm_statistics64_data_t vm;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    vm_size_t page = 0;

    if (sysctlbyname("hw.memsize", &memsize, &len, NULL, 0) < 0 || memsize == 0)
        return;
    if (host_page_size(host, &page) != KERN_SUCCESS)
        return;
    if (host_statistics64(host, HOST_VM_INFO64, (host_info64_t)&vm, &count) != KERN_SUCCESS)
        return;

    uint64_t app = (uint64_t)(vm.internal_page_count - vm.purgeable_count);
    uint64_t used = (app + vm.wire_count + vm.compressor_page_count) * (uint64_t)page;

    metrics_set("mem_total_bytes", (double)memsize);
    metrics_set("mem_used_bytes", (double)used);
    metrics_set("mem_used_percent", 100.0 * (double)used / (double)memsize);

    struct xsw_usage swap;
    len = sizeof swap;
    if (sysctlbyname("vm.swapusage", &swap, &len, NULL, 0) == 0) {
        metrics_set("swap_total_bytes", (double)swap.xsu_total);
        metrics_set("swap_used_bytes", (double)swap.xsu_used);
    }
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
    host = mach_host_self();

    metrics_set_help("cpu_usage_percent", "CPU busy time over the last interval, in percent.");
    metrics_set_help("mem_total_bytes", "Physical memory.");
    metrics_set_help("mem_used_bytes", "Memory in use (app + wired + compressed).");
    metrics_set_help("mem_used_percent", "Memory in use, in percent.");
    metrics_set_help("swap_total_bytes", "Swap space.");
    metrics_set_help("swap_used_bytes", "Swap space in use.");
    metrics_set_help("load1", "Load average over 1 minute.");
    metrics_set_help("load5", "Load average over 5 minutes.");
    metrics_set_help("load15", "Load average over 15 minutes.");

    unsigned long long idle, total;
    if (read_cpu(&idle, &total) < 0) {
        log_err("host_statistics failed");
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
