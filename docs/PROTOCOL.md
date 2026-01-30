# Control protocol

`sysmond` listens on a unix stream socket (`[core] socket`, default
`/tmp/sysmond.sock`). See [ADR 0002](adr/0002-unix-socket-protocol.md).

* The client connects and sends **one** command terminated by `\n`.
* The server writes the reply and closes the connection.
* Words are separated by spaces. Commands are case sensitive.
* A reply starting with `ERR ` is an error; the rest of the line says why.
* Numbers are printed with `%g`; `nan` means "no sample yet".

Any tool that speaks unix sockets works:

    printf 'list\n' | nc -U /tmp/sysmond.sock
    socat - UNIX-CONNECT:/tmp/sysmond.sock <<< 'get load1'

## Commands

| command | reply |
|---|---|
| `ping` | `pong` |
| `list` | one `NAME VALUE` line per metric |
| `get METRIC` | the current value |
| `history METRIC [N]` | the last N samples, oldest first, one per line (default 60, max 300) |
| `avg METRIC [N]` | average of the last N samples |
| `stats METRIC [N]` | `min`, `max`, `avg`, `p50`, `p95` of the last N samples and the `last` value, one `KEY VALUE` per line |
| `json` | all metrics as one JSON object: `{"version":..., "timestamp":..., "metrics":{...}}` |
| `alerts` | one line per rule: `FIRING` or `ok`, the rule, last value, seconds since last change |
| `reload` | re-read the configuration file, same as `SIGHUP`; replies `reloading` |
| `version` | `sysmond X.Y.Z` |
| `help` | the list of commands |

## Limits

* Requests longer than 511 bytes are truncated.
* A client has one second to send its line, after which the server
  answers whatever it got so far (usually `ERR empty command`).
