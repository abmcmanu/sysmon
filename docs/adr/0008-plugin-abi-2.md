# 8. Plugin ABI 2: options

* Status: accepted
* Date: 2026-04

## Context

Plugins had no way to receive settings. `net` summed every interface,
including docker bridges and VPN tunnels; `diskio` reported dozens of
device-mapper volumes on some hosts; `cpustat` produced 128 per-core
metrics on a big box. Each needed a knob.

## Decision

Add one function at the end of `sysmon_host_t`:

```c
const char *(*option)(const char *plugin, const char *key);
```

backed by `[plugin.NAME]` sections in the configuration, and bump
`SYSMON_PLUGIN_ABI` to 2.

Because the field is appended, the table is a superset of ABI 1: the
daemon keeps loading ABI 1 plugins unchanged. An ABI 2 plugin checks
`host->abi >= 2` before calling `option()` and so still runs on a 3.x
daemon, with its defaults.

## Consequences

* Option values are plain strings; each plugin parses its own.
* Options are read at `init()`; changing them needs a restart, like
  the rest of `[plugins]`.
* Future additions follow the same rule: append, bump, check `abi`.
