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

Build it like the bundled ones:

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

## Bundled plugins

| plugin | metrics | platforms |
|---|---|---|
| `uptime` | `uptime_seconds` | Linux, macOS |
| `net` | `net_rx_bytes_per_second`, `net_tx_bytes_per_second` | Linux, macOS |
