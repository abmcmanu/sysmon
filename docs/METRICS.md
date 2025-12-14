# Metrics

Every metric is a gauge: a number sampled once per interval.
`sysmonctl list` shows the current values, Prometheus sees the same
names with a `sysmon_` prefix.

## Naming

* lower case, words separated by `_`, `[a-zA-Z_][a-zA-Z0-9_]*`
* the unit is the last word: `_bytes`, `_seconds`, `_percent`,
  `_celsius`, `_per_second`
* the first word is the subsystem: `cpu_`, `mem_`, `disk_`, `net_`...
