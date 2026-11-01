# Writing a plugin

A plugin is a shared object that adds metrics. The whole API is
[`include/sysmon/plugin.h`](../include/sysmon/plugin.h); the design is
explained in [ADR 0003](adr/0003-plugin-abi.md).

## Minimal example

```c
/* plugins/hello.c */
#include "sysmon/plugin.h"

static const sysmon_host_t *H;

static int hello_init(const sysmon_host_t *host)
{
    H = host;
    H->set_help("hello_answer", "The answer.");
    return 0;
}

static int hello_collect(void)
{
    H->set("hello_answer", 42);
    return 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "hello", "1.0", hello_init, hello_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
```

The file is in [`examples/plugins/hello.c`](../examples/plugins/hello.c),
`make examples` builds it. By hand:

    cc -std=c11 -Iinclude -fPIC -shared -o build/plugins/hello.so plugins/hello.c

and enable it:

    [plugins]
    dir = /path/to/build/plugins
    load = hello

## Rules

* `collect()` runs on the daemon's only thread, once per interval. Keep
  it under a millisecond or so; never block on the network.
* Return non-zero from `collect()` on failure. After 5 consecutive
  failures the plugin is disabled until the daemon restarts.
* Return non-zero from `init()` when the plugin cannot work on this
  machine (missing file, wrong OS): it is unloaded with a warning.
* Metric names must match `[a-zA-Z_][a-zA-Z0-9_]*`. Use a prefix
  (`net_`, `gpu_`...) and put the unit in the name (`_bytes`,
  `_seconds`, `_percent`).
* Do not call `exit()`, install signal handlers or spawn threads.

## Options (ABI 2)

A `[plugin.NAME]` section passes settings to a plugin:

```ini
[plugin.hello]
greeting = salut
```

```c
static int hello_init(const sysmon_host_t *host)
{
    H = host;
    const char *g = H->abi >= 2 ? H->option("hello", "greeting") : NULL;
    ...
}
```

Check `host->abi` first: the same plugin then still loads in a 3.x
daemon, which has no `option()`. The daemon loads plugins built for
ABI 1 and 2.

Options of the bundled plugins:

| plugin | key | |
|---|---|---|
| `cpustat` | `per_core` | `no` to skip the `cpuN_usage_percent` metrics on big machines |
| `net` | `interfaces` | comma-separated list of interfaces to sum; default: all but loopback and virtual ones (veth, docker, bridges, utun...) |
| `diskio` | `devices` | comma-separated list of devices, default all whole disks |
| `cgroup` | `path` | cgroup directory, default the one sysmond runs in |

## Bundled plugins

| plugin | metrics | platforms |
|---|---|---|
| `sockets` | `tcp_inuse`, `tcp_time_wait`, `tcp_orphan`, `udp_inuse`, `tcp6_inuse`, `udp6_inuse`, `tcp_retransmits_per_second` | Linux |
| `conntrack` | `conntrack_entries`, `conntrack_max`, `conntrack_used_percent` | Linux with nf_conntrack |
| `battery` | `battery_percent`, `battery_charging`, `battery_power_watts` | Linux |
| `cgroup` | `cgroup_memory_bytes`, `cgroup_memory_limit_bytes`, `cgroup_memory_percent`, `cgroup_cpu_usage_percent`, `cgroup_cpu_throttled_per_second`, `cgroup_pids` | Linux, cgroup v2 |
| `uptime` | `uptime_seconds` | Linux, macOS |
| `net` | `net_rx_bytes_per_second`, `net_tx_bytes_per_second` | Linux, macOS |
| `cpustat` | `cpu_count`, `cpuN_usage_percent`, `cpu_{user,system,iowait,steal}_percent`, `context_switches_per_second`, `interrupts_per_second` | Linux, macOS (no iowait, steal, context switches) |
| `procs` | `procs_total`, `procs_running`, `procs_blocked`, `threads_total`, `forks_per_second` | Linux, macOS (`procs_total` only) |
| `fd` | `fd_used`, `fd_max`, `fd_used_percent` | Linux, macOS |
| `diskio` | `disk_<dev>_{read,write}_bytes_per_second`, `disk_<dev>_iops`, `disk_<dev>_busy_percent`, totals | Linux |
| `psi` | `psi_{cpu,memory,io}_{some,full}_avg{10,60}` | Linux 4.20+ |
| `thermal` | `temp_<zone>_celsius`, `temp_max_celsius`; falls back to hwmon sensors | Linux |
