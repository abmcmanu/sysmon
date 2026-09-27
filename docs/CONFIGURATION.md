# Configuration

`sysmond -c FILE` reads an ini-style file. `sysmond -t -c FILE` checks
it without starting.

```ini
# comment
; also a comment
[section]
key = value
```

* A `#` or `;` after whitespace starts a comment, also at the end of a
  line. Put the value in double quotes to keep them: `"a # b"`.
* Keys before the first section belong to `[core]`.
* Unknown keys and invalid values are errors: the daemon refuses to start,
  and a reload keeps the previous configuration.
* `mount`, `load` and `rule` may be repeated; the others keep the last value.

### Durations

`interval` accepts a number of seconds or a suffix: `30`, `30s`, `5m`,
`1h`.

### Environment variables

`${NAME}` is replaced by the variable's value; `${NAME:-default}` uses
`default` when it is unset. An unset variable without a default is an
error, so typos do not silently turn into empty strings.

```ini
socket = ${RUNTIME_DIRECTORY:-/tmp}/sysmond.sock
```

### Includes

`include = PATTERN` reads every file matching the glob, in alphabetical
order, as if its content was pasted there. Relative patterns are
relative to the including file. A pattern matching nothing is fine.

```ini
include = sysmond.d/*.conf
```

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

## [graphite]

| key | default | |
|---|---|---|
| `host` | none | carbon host |
| `port` | `0` (disabled) | carbon plaintext port, usually 2003 |
| `prefix` | `sysmon.<hostname>` | path of every metric, dots in the host name become `_` |

Every sample of every metric is sent over UDP, cut into datagrams of at
most 1400 bytes. There is no retry: a lost datagram is a gap.

## [alerts]

| key | default | |
|---|---|---|
| `rule` | none | `METRIC OP THRESHOLD [for N]`, repeat for several |
| `exec` | none | program run on every state change |

`OP` is one of `>`, `>=`, `<`, `<=`. `for N` requires N consecutive
samples before firing.
`for 30s`, `for 5m` give the same as a duration, rounded up to whole
samples.

## Reloading

`SIGHUP` or `sysmonctl reload` re-reads the file.

| applied on reload | needs a restart |
|---|---|
| `interval`, `log_level`, `log_file`, `mount`, `rule`, `exec` | `socket`, `socket_mode`, `user`, `[plugins]`, `[http]` |
