# Configuration

`sysmond -c FILE` reads an ini-style file. `sysmond -t -c FILE` checks
it without starting.

```ini
# comment
; also a comment
[section]
key = value
```

* Keys before the first section belong to `[core]`.
* Unknown keys and invalid values are errors: the daemon refuses to start,
  and a reload keeps the previous configuration.
* `mount`, `load` and `rule` may be repeated; the others keep the last value.

## [core]

| key | default | |
|---|---|---|
| `interval` | `5` | time between two samples |
| `log_level` | `info` | `error`, `warn`, `info` or `debug` |
| `log_file` | stderr | a path, or `syslog` |
| `pid_file` | none | written when daemonized |
| `socket` | `/tmp/sysmond.sock` | control socket |
| `socket_mode` | `0660` | permissions of the socket |
| `user` | none | user to switch to after startup (when started as root) |

## [disk]

| key | default | |
|---|---|---|
| `mount` | `/` | mount point to watch; repeat for several |

## [plugins]

| key | default | |
|---|---|---|
| `dir` | `/usr/local/lib/sysmon/plugins` | where `NAME.so` files are |
| `load` | none | plugin to load; repeat for several |

## [http]

| key | default | |
|---|---|---|
| `port` | `0` (disabled) | Prometheus / API port |
| `bind` | `127.0.0.1` | address to listen on |
