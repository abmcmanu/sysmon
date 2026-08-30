# Changelog

All notable changes to sysmon. Dates are release dates.

## 4.4.0 - 2026-08-30

- Configuration: comments at the end of a line, double-quoted values.
- `sysmonctl` exits with 3 when the daemon is unreachable
- `-V` also shows the daemon version.
- `thermal` falls back to hwmon sensors
- `psi` reports irq pressure
- `procs_zombie`.
- `net` skips virtual interfaces unless listed.
- JSON output and alert hooks carry the host name.
- Requests longer than 511 bytes are refused instead of truncated.
- Warning when a plugin's collect() takes more than 100 ms.
- Fixed: the same dropped metric was logged on every tick.

## 4.3.0 - 2026-07-20

- FreeBSD support (collector, plugins, CI).
- `sysmonctl(1)` man page.
- Operations guide, FAQ, contributing and security notes
- alert hook examples.

## 4.2.0 - 2026-06-14

- Docker image, and a compose stack with Prometheus and Grafana (dashboard included).
- Prometheus alerting rules in `contrib/prometheus/alerts.yml`.
- Packaging for Homebrew, Debian, Fedora and Arch
- `make dist`.
- Fuzzing targets (`make fuzz`), micro benchmarks (`make bench`).
- CI: gcc and clang, musl, CodeQL, release workflow.

## 4.1.0 - 2026-04-22

- `sysmonctl -w SEC` refreshes a command like watch(1).
- Shell completions for bash, zsh and fish, installed by `make install`.
- `top`: help screen, pause, filter, sorted list, `NO_COLOR`.

## 4.0.0 - 2026-04-02

- Plugin ABI 2: `[plugin.NAME]` options passed to plugins. ABI 1 plugins keep working.
- `cpustat per_core`, `net interfaces`, `diskio devices` options.
- HTTP listener on IPv6 addresses.
- Alert rules accept durations: `for 30s`, `for 5m`.

## 3.3.0 - 2026-03-15

- Web dashboard at `/` (`[http] port`).
- JSON API: `/api/metrics`, `/api/history`, `/api/version`.
- HTTP responses for `/api` are not cached.

## 3.2.0 - 2026-02-11

- Configuration: durations (`5m`), `${VAR}` expansion, `include` with globs.
- `stats` command (min, max, avg, p50, p95).
- `list PATTERN`.
- `diskio`, `psi` and `thermal` plugins
- inode usage for disks.
- `log_file = syslog`
- `[core] user` to drop privileges.

## 3.1.0 - 2025-12-12

- `cpustat`, `procs` and `fd` plugins.
- `[core] socket_mode`.
- Values in `list` and `get` keep 15 significant digits.
- `get` without a metric name now answers with a usage error.
- Disk metric names no longer end with `_` for mount points with a trailing slash.

## 3.0.1 - 2025-09-06

- `top` shows disks, network throughput and uptime.

## 3.0.0 - 2025-03-22

- `sysmonctl top`: full-screen dashboard with bars and sparklines.

## 2.5.0 - 2024-05-04

- `make install`, systemd unit, launchd plist, man page.

## 2.4.0 - 2023-06-10

- Live reload with SIGHUP or `sysmonctl reload`; alert state survives reloads.
- `-t` checks the configuration.
- The log file is reopened on reload (logrotate).

## 2.3.1 - 2022-07-02

- Alert exec hook (`[alerts] exec`).
- Fixed: finished hooks were left as zombies.

## 2.3.0 - 2022-03-19

- Alert rules with hysteresis (`for N`), `alerts` command, `alerts_firing` gauge.
- `json` command.
- HTTP: 405 for non-GET requests.

## 2.2.0 - 2021-05-01

- Prometheus endpoint (`[http] port`), `/healthz`.
- Metric names are validated.

## 2.1.0 - 2020-10-03

- `poll()` based event loop.
- Unit tests, smoke test, sanitizers, CI on Linux and macOS.
- Fixed: `avg` returned 0 when asked for more samples than available.

## 2.0.1 - 2019-06-15

- Fixed: absurd network rates when macOS 32-bit counters wrapped.

## 2.0.0 - 2019-04-13

- Plugin ABI (`include/sysmon/plugin.h`) and `dlopen` loader.
- `uptime` and `net` plugins.
- Control socket: 1 s read timeout.

## 1.3.1 - 2018-08-25

- Fixed: on macOS, accepted sockets inherited O_NONBLOCK from the listener and short requests were dropped.

## 1.3.0 - 2018-05-26

- macOS support.

## 1.2.0 - 2017-12-02

- 300-sample history per metric, `history` and `avg` commands.
- Refuse to start when another sysmond is listening on the socket.
- Unknown configuration keys are now errors.

## 1.1.0 - 2017-05-20

- Unix socket control protocol (`ping`, `list`, `get`, `help`, `version`).
- `sysmonctl` client.
- Samples are printed only with `-p`.

## 1.0.0 - 2016-10-02

- Rewrite as a C daemon, `sysmond`. The shell version moves to `legacy/`.
- cpu, memory, swap, load and disk usage.
- ini configuration file, daemon mode, pid file, log file.
- Fixed sampling cadence on the monotonic clock.

## 0.5.0 - 2015-06-06

- Swap usage.
- Test suite (bats), Makefile, Travis CI.

## 0.4.0 - 2014-11-22

- Memory usage based on `MemAvailable` (Linux >= 3.14).
- Alerts are also sent to syslog.
- shellcheck clean.

## 0.3.0 - 2014-03-29

- CSV output `-C` for spreadsheets and gnuplot.
- Clear error on systems without /proc.

## 0.2.0 - 2013-09-21

- Filesystem usage and top processes.
- Alert thresholds `-c` / `-m`, sample log `-l`.
- Fixed: cpu usage counted iowait as busy time.

## 0.1.0 - 2013-04-07

- First release: cpu, memory and load with bars and colors.
- `-i SEC` refresh mode.
