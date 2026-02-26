# HTTP endpoints

Enabled with `[http] port`. Plain HTTP/1.0, GET only, one request per
connection, no authentication: bind to localhost (the default) or put a
reverse proxy in front.

| path | content |
|---|---|
| `/` | a small HTML page |
| `/metrics` | Prometheus text format, every metric prefixed with `sysmon_` |
| `/healthz` | `ok`, for load balancers and container probes |
| `/api/metrics` | all metrics as JSON, same shape as `sysmonctl json` |
| `/api/history?metric=NAME&n=N` | `{"metric": NAME, "values": [...]}`, oldest first (default 60, max 300) |
| `/api/version` | `{"version": "X.Y.Z"}` |

`/api/*` answers carry `Cache-Control: no-store`.
