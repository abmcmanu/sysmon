# Performance

sysmond is meant to be invisible on the machine it watches.

## Footprint

| | |
|---|---|
| binary | ~150 KB stripped, plus ~20 KB per plugin |
| RSS | ~1 MB, of which ~600 KB is the metric registry (256 metrics x 300 samples) |
| cpu | well under 0.1 % of a core at a 5 s interval with every plugin |
| threads | 1 |
| allocations per tick | none in the sampling path |

## Micro benchmarks

`make bench` times the hot paths. On a 2021 laptop (Ryzen 7 5800U, gcc
13, `-O2`):

```
ring_push                           1.6 ns/op
ring_avg(300)                     310.0 ns/op
ring_percentile(300)             9800.0 ns/op
metrics_set (200 metrics)         180.0 ns/op
server list (200 metrics)       21000.0 ns/op
json_metrics (200 metrics)      34000.0 ns/op
```

A tick with the core collectors and the bundled plugins takes about
100 microseconds, dominated by reading `/proc`.

## What it costs to look

Every `sysmonctl` call is one connection and one request. `sysmonctl top`
asks for `list` plus two `history` per refresh. Prometheus scrapes are a
single render of the registry: about 50 microseconds for 200 metrics.
