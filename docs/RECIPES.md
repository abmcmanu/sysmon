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
