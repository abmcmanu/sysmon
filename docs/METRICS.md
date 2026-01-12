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

## Disks

For every `[disk] mount`, the mount point becomes part of the name:
`/` is `root`, `/var/lib` is `var_lib`.

| metric | meaning |
|---|---|
| `disk_<mount>_used_percent` | used space, like the `Use%` column of `df` |
| `disk_<mount>_free_bytes` | space available to unprivileged users |
| `disk_<mount>_inodes_used_percent` | inodes in use; absent on file systems without an inode table (btrfs, zfs) |

## Alerts

| metric | meaning |
|---|---|
| `alerts_firing` | number of rules currently firing |

## Plugins

Each plugin documents its metrics in [PLUGINS.md](PLUGINS.md).

## Units and precision

Values are doubles. Rates (`_per_second`) are computed from counter
differences over the real elapsed time, not the configured interval, so
a late tick does not create a spike. Counters that go backwards (reboot,
wrap, device removed) skip one sample instead of reporting garbage.
