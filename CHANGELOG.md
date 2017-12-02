# Changelog

All notable changes to sysmon. Dates are release dates.

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
