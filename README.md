# sysmon

**A small, dependency-free system monitoring daemon for Linux and macOS.**

`sysmond` samples cpu, memory, swap, load, disks and anything its
plugins provide, keeps a short history in memory, evaluates alert rules
and serves it all over a unix socket and a Prometheus endpoint.
`sysmonctl` is the command line client, with a `top`-like dashboard.

```
sysmon top  web01  21:42:07  every 1.0s  q quit  +/- speed

  CPU    23.4% ███████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░  ▂▂▃▅▇▆▃▂▂▁▁▂▃▃▂▂▄▆▇▅▃
  MEM    61.2% ██████████████████████████░░░░░░░░░░░░░░  ▅▅▅▅▅▅▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆
        9.8 GiB / 16.0 GiB
  LOAD  1.12 0.97 0.85

  root   48.0% ███████████████████░░░░░░░░░░░░░░░░░░░░░
  NET   ↓ 1.2 MiB/s   ↑ 88.4 KiB/s
  UP    12d 04:17
```

* ~3k lines of C11 + POSIX, no dependency beyond libc
* one thread, one `poll()` loop, no allocation while sampling
* plugin ABI (`dlopen`), Prometheus exporter, JSON, alert hooks
* started in 2013 as a bash script, still around in [`legacy/`](legacy/)

## Quick start

```sh
make                       # build/sysmond, build/sysmonctl, build/plugins/*.so
make test smoke            # unit + end-to-end tests

./build/sysmond -f -c contrib/sysmond.conf &
./build/sysmonctl top
```

Linux (glibc or musl) and macOS (Xcode command line tools) are tested in
CI.
systemd and launchd are in [`contrib/`](contrib/).

## Using it

```sh
sysmonctl list                          # every metric
sysmonctl get mem_used_percent          # one value, for scripts
sysmonctl history cpu_usage_percent 60  # last 60 samples
sysmonctl avg load1 12                  # average of the last 12
sysmonctl list 'disk_*'                 # filter with a glob
sysmonctl json                          # everything, machine-readable
sysmonctl alerts                        # rule states
sysmonctl reload                        # same as kill -HUP
```

No client needed if you prefer: `printf 'get load1\n' | nc -U /tmp/sysmond.sock`.
The protocol is in [docs/PROTOCOL.md](docs/PROTOCOL.md).

### Prometheus

```ini
[http]
port = 9273          ; bind defaults to 127.0.0.1
```

```yaml
scrape_configs:
  - job_name: sysmon
    static_configs:
      - targets: ['web01:9273', 'web02:9273']
```

### Alerts

```ini
[alerts]
rule = cpu_usage_percent > 90 for 6
rule = disk_root_used_percent >= 90
exec = /usr/local/bin/notify-me      ; gets SYSMON_ALERT, SYSMON_STATE, ...
```

### Plugins

```ini
[plugins]
dir  = /usr/local/lib/sysmon/plugins
load = net
load = uptime
```

Writing one is ~30 lines of C against a single header,
see [docs/PLUGINS.md](docs/PLUGINS.md).

## Metrics

| name | source |
|---|---|
| `cpu_usage_percent` | `/proc/stat`, Mach `HOST_CPU_LOAD_INFO` |
| `mem_total_bytes`, `mem_used_bytes`, `mem_used_percent` | `/proc/meminfo` (`MemAvailable`), `HOST_VM_INFO64` |
| `swap_total_bytes`, `swap_used_bytes` | `/proc/meminfo`, `vm.swapusage` |
| `load1`, `load5`, `load15` | `getloadavg(3)` |
| `disk_<mount>_used_percent`, `disk_<mount>_free_bytes` | `statvfs(3)` |
| `alerts_firing` | alert engine |
| `net_rx_bytes_per_second`, `net_tx_bytes_per_second` | `net` plugin |
| `uptime_seconds` | `uptime` plugin |
| `cpuN_usage_percent`, `cpu_*_percent`, `context_switches_per_second` | `cpustat` plugin |
| `procs_total`, `procs_running`, `threads_total`, `forks_per_second` | `procs` plugin |
| `fd_used`, `fd_max`, `fd_used_percent` | `fd` plugin |
| `disk_<dev>_read_bytes_per_second`, `disk_<dev>_iops`, ... | `diskio` plugin |
| `psi_*_avg10`, `psi_*_avg60` | `psi` plugin |
| `temp_*_celsius` | `thermal` plugin |

## Documentation

* [Architecture](docs/ARCHITECTURE.md): modules, loop, boundaries
* [Decision records](docs/adr/): why C, why a unix socket, the plugin
  ABI, the poll loop, Prometheus, the TUI without ncurses
* [Metrics](docs/METRICS.md): names, units, where they come from
* [Protocol](docs/PROTOCOL.md) and [plugins](docs/PLUGINS.md)
* `man docs/sysmond.8`
* [Changelog](CHANGELOG.md): thirteen years of it

## History

| | |
|---|---|
| 2013 | `sysmon.sh`: bars, colors, alerts, logging, CSV |
| 2016 | rewrite as a C daemon (1.0) |
| 2017 | unix socket protocol, `sysmonctl`, history ring buffers |
| 2018 | macOS support |
| 2019 | plugin ABI (2.0) |
| 2020 | tests, sanitizers, CI, `poll()` event loop |
| 2021 | Prometheus exporter |
| 2022 | JSON, alert rules and hooks |
| 2023 | live reload (SIGHUP) |
| 2024 | packaging: systemd, launchd, man page |
| 2025 | `sysmonctl top` (3.0) |

## License

MIT
