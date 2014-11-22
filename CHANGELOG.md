# Changelog

All notable changes to sysmon. Dates are release dates.

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
