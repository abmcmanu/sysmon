# Changelog

All notable changes to sysmon. Dates are release dates.

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
