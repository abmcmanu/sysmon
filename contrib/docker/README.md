# Docker

Build the image:

    docker build -t sysmon .

Run it against the host, not against its own container:

    docker run -d --name sysmon --pid=host --network=host \
        -v /:/rootfs:ro -e SYSMON_DISK=/rootfs sysmon

* `--pid=host` and `--network=host` make `/proc` and the interface
  counters describe the host.
* the root file system is mounted read-only at `/rootfs` for disk usage.
* `SYSMON_INTERVAL`, `SYSMON_LOG_LEVEL` and `SYSMON_DISK` are read by
  [sysmond.conf](sysmond.conf).

Then `curl localhost:9273/metrics`, or open `http://localhost:9273/`.

## With Prometheus and Grafana

[`compose.yml`](compose.yml) starts sysmond, Prometheus scraping it every
15 seconds and Grafana with a ready-made dashboard:

    docker compose -f contrib/docker/compose.yml up -d

Grafana is on http://localhost:3000 (admin / admin), the dashboard is
"sysmon".
