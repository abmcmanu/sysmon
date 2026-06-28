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
