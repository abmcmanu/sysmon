# 7. Embedded web dashboard

* Status: accepted
* Date: 2026-02

## Context

People who do not run Prometheus and Grafana still want to look at a
machine from a browser. `sysmonctl top` needs a shell on the host.

## Decision

Serve one static HTML page at `/`, written by hand: inline CSS and
JavaScript, no framework, no external requests. It polls `/api/metrics`
and draws sparklines from `/api/history` on a canvas.

The file lives in `web/index.html` and is turned into a C byte array at
build time by `scripts/embed.sh` (plain `od` and `awk`, no `xxd`).

## Consequences

* The daemon gains about 10 KB and no runtime dependency.
* No authentication, like the rest of the HTTP listener: keep it bound
  to localhost or behind a proxy.
* Editing the page means rebuilding the daemon. Acceptable for a page
  that changes a few times a year.
