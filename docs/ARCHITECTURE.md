# Architecture

sysmon is a small daemon (`sysmond`) and its client (`sysmonctl`).
This page is the map; the *why* of each choice lives in
[`docs/adr/`](adr/).

## Overview

```
                        ┌──────────────────────── sysmond ─────────────────────────┐
                        │                                                          │
   /proc, sysctl,  ───▶ │  collectors ──┐                                          │
   mach, statvfs        │  (per OS)     │                    ┌──▶ server.c ────────┼──▶ unix socket ──▶ sysmonctl
                        │               ├──▶ metrics.c ──────┤    (line protocol)  │                    (list, get, top...)
   plugins/*.so    ───▶ │  plugins.c ───┘    registry +      │                     │
   (dlopen)             │                    ring buffers    ├──▶ http.c ──────────┼──▶ :9273/metrics ──▶ Prometheus
                        │                        │           │    (exposition)     │
                        │                        ▼           └──▶ json.c           │
                        │                    alert.c ──────────▶ log, exec hook    │
                        │                                                          │
                        │  main.c: config, signals, poll() loop                    │
                        └──────────────────────────────────────────────────────────┘
```

## The loop

Everything happens on one thread ([ADR 0004](adr/0004-poll-loop.md)):

1. **tick**: the platform collector, the disk collector and every
   plugin write gauges into the registry; alert rules are evaluated on
   the new values.
2. **wait**: until the next tick, `poll()` on the listeners (control
   socket, HTTP) and serve one request per ready descriptor.

Signals only set flags (`running`, `reload_requested`) that the loop
checks; nothing is done in signal context.

## Modules

| file | role |
|---|---|
| `src/main.c` | options, startup order, signals, the loop, reload |
| `src/config.c` | ini parser; strict: unknown keys are errors |
| `src/metrics.c` | registry of named gauges, each with a ring buffer of history |
| `src/ring.c` | fixed-size circular buffer (300 samples), no allocation |
| `src/collect_linux.c` | cpu/mem/swap/load from `/proc` |
| `src/collect_darwin.c` | same from Mach `host_statistics` and `sysctl` |
| `src/collect_freebsd.c` | same from `sysctl` (`kern.cp_time`, `vm.stats`) |
| `src/collect_disk.c` | `statvfs()` on configured mount points, portable |
| `src/plugins.c` | `dlopen` loader, ABI check, failure counting |
| `src/server.c` | control socket and its commands |
| `src/http.c` | HTTP/1.0 listener, Prometheus exposition |
| `src/json.c` | JSON rendering of the registry |
| `src/alert.c` | threshold rules, state machine, exec hook |
| `src/log.c`, `src/sb.c` | logging, growable string buffer |
| `tools/sysmonctl.c` | CLI client |
| `tools/tui.c` | `sysmonctl top` |
| `include/sysmon/plugin.h` | the only public header |

## Boundaries

* Collectors and plugins only *write* the registry; listeners only
  *read* it. Neither knows about the other.
* Plugins see `include/sysmon/plugin.h` and nothing else. The daemon can
  be refactored freely without breaking them.
* Platform code is confined to `collect_<os>.c` (and `#ifdef` blocks in
  plugins). Everything else is POSIX.
* `sysmonctl` shares only `sb.c` with the daemon; it talks to it through
  the documented protocol, like any third-party client would.

## Memory

No allocation in the sampling path. The registry is a static array of
`METRICS_MAX` (256) metrics, each embedding its 300-sample ring: about
600 KB, fixed at startup. String buffers used to build replies are
allocated per request and freed right after.

## Startup order

    parse options → load config → open log → collectors init → alert rules
    → open listeners → load plugins → daemonize → pid file → loop

Listeners and plugins are opened before `daemonize()` so that errors
still reach the terminal and relative paths are resolved against the
directory the daemon was started from.

## Testing

* `make test`: unit tests (`tests/test_*.c`), one binary each, linked
  against the daemon objects.
* `make smoke`: starts a real daemon with a temporary config and drives
  it with `sysmonctl` and `curl`.
* `make asan`: both of the above under AddressSanitizer + UBSan.
* CI runs everything on Linux and macOS.
