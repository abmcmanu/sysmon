# 5. Expose a Prometheus endpoint instead of pushing metrics

* Status: accepted
* Date: 2021-03-13

## Context

People want sysmon numbers in dashboards across many hosts. The two
models are push (daemon sends to InfluxDB, Graphite, a Pushgateway...)
and pull (a server scrapes each host).

## Decision

Pull, using the Prometheus text exposition format on
`GET /metrics`, served by a minimal built-in HTTP/1.0 server on its own
listener (disabled by default, `[http] port`).

* One format understood by Prometheus, VictoriaMetrics, Grafana Agent,
  the OpenTelemetry collector, Datadog...
* No outbound connections, no credentials stored on monitored hosts.
* The scraper decides the resolution; the daemon keeps its own interval.

The HTTP server is deliberately tiny: GET only, one request per
connection, no keep-alive, no TLS, 1 s read timeout. All metrics are
gauges prefixed with `sysmon_`; metric names are validated against the
Prometheus charset when they are registered.

## Consequences

* No authentication: bind to 127.0.0.1 (the default) or put a reverse
  proxy in front.
* Hosts behind NAT need a pull-friendly path (VPN, agent). Push stays
  possible later as a plugin.
