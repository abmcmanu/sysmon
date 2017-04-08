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
| `version` | `sysmond X.Y.Z` |
| `help` | the list of commands |

## Limits

* Requests longer than 511 bytes are truncated.
