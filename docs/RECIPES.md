# Recipes

Short answers to "how do I...".

## Get an alert before a disk is full

```ini
[alerts]
rule = disk_root_used_percent >= 90 for 10m
exec = /usr/local/bin/notify-me
```

With Prometheus, `predict_linear` sees it coming days ahead: see
`DiskFillingUp` in `contrib/prometheus/alerts.yml`.

## Watch one network interface

```ini
[plugins]
load = net

[plugin.net]
interfaces = eth0
```

## Monitor a container from inside

Load the `cgroup` plugin: it finds the container's own cgroup and
reports its memory against the limit, which is what the OOM killer
looks at, not `mem_used_percent` of the host.

```ini
[plugins]
load = cgroup
```

## Raspberry Pi temperature

```ini
[plugins]
load = thermal

[alerts]
rule = temp_max_celsius > 75 for 2m
```

## A CSV of the last five minutes

```sh
for m in cpu_usage_percent mem_used_percent load1; do
    sysmonctl history "$m" 60 | paste -sd, - | sed "s/^/$m,/"
done
```

## Feed Telegraf or InfluxDB

```toml
[[inputs.http]]
  urls = ["http://127.0.0.1:9273/api/influx"]
  data_format = "influx"
```
