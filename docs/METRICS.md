# Metrics

Every metric is a gauge: a number sampled once per interval.
`sysmonctl list` shows the current values, Prometheus sees the same
names with a `sysmon_` prefix.

## Naming

* lower case, words separated by `_`, `[a-zA-Z_][a-zA-Z0-9_]*`
* the unit is the last word: `_bytes`, `_seconds`, `_percent`,
  `_celsius`, `_per_second`
* the first word is the subsystem: `cpu_`, `mem_`, `disk_`, `net_`...

## Core

These come from the daemon itself, on every platform.

| metric | meaning |
|---|---|
| `cpu_usage_percent` | busy time over the last interval, all cpus |
| `mem_total_bytes` | physical memory |
| `mem_used_bytes` | memory in use: total minus available (Linux), app + wired + compressed (macOS) |
| `mem_used_percent` | the same in percent |
| `swap_total_bytes`, `swap_used_bytes` | swap space |
| `load1`, `load5`, `load15` | load averages |
