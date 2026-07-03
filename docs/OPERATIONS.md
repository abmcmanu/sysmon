# Running sysmond in production

## Installing

```sh
make && sudo make install          # /usr/local/{bin,lib/sysmon,share/man}
sudo install -m 644 contrib/sysmond.conf /etc/sysmond.conf
```

## systemd

`contrib/sysmond.service` runs the daemon in the foreground under a
dynamic user with a read-only view of the system:

```sh
sudo cp contrib/sysmond.service /etc/systemd/system/
sudo systemctl enable --now sysmond
sudo systemctl reload sysmond      # SIGHUP: re-read the config
```

With `DynamicUser=yes`, `/tmp` is private to the service: put the
socket in `/run/sysmond/sysmond.sock` and point `sysmonctl -s` (or
`SYSMON_SOCKET`) at it.

## Without systemd

```sh
sysmond -c /etc/sysmond.conf      # daemonizes, writes pid_file if set
kill -HUP "$(cat /run/sysmond.pid)"
```

Start it as root with `[core] user = nobody` if the socket or the pid
file live in root-owned directories: it opens them, then drops
privileges.

## Logs

`log_file` takes a path or `syslog`. With a file, rotate it with
logrotate and send `SIGHUP` afterwards:

```
/var/log/sysmond.log {
    weekly
    rotate 4
    compress
    postrotate
        kill -HUP "$(cat /run/sysmond.pid)" 2> /dev/null || true
    endscript
}
```

## Monitoring the monitor

* `/healthz` answers `ok` as long as the loop runs.
* `sysmon_build_info` tells Prometheus which version runs where.
* `contrib/prometheus/alerts.yml` has a `SysmonDown` rule.

## Alert hooks

The hook gets the alert in its environment and should return quickly;
it runs in the background but a hook stuck forever is a leaked process.

```sh
#!/bin/sh
# /usr/local/bin/sysmon-notify
curl -fsS -m 5 -d "$(hostname): $SYSMON_ALERT is $SYSMON_STATE ($SYSMON_VALUE)" \
    https://ntfy.sh/my-servers
```

More examples in [`contrib/hooks/`](../contrib/hooks/).

## Upgrading from 3.x to 4.x

* Plugins built for 3.x (ABI 1) keep working.
* Rules written as `for N` keep their meaning; `for 30s` is new.
* Nothing in the configuration was removed.
