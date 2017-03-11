# 2. Line-based protocol over a unix socket

* Status: accepted
* Date: 2017-03-11

## Context

`sysmond` keeps metrics in memory; something has to read them. Options:

1. Write a file every tick (`/run/sysmond/metrics`): trivial but racy,
   wastes I/O and gives no way to ask for history.
2. HTTP + JSON: needs a parser and an HTTP implementation, opens a
   network port on every box.
3. A unix domain socket with a tiny text protocol.

## Decision

Option 3. One command per connection, one line in, a few lines out,
the server closes the connection when done. Errors start with `ERR`.

    $ printf 'get load1\n' | nc -U /tmp/sysmond.sock
    0.42

* Access control is plain file permissions on the socket.
* Debuggable with `nc -U` or `socat`, no client library needed.
* One request per connection keeps the server stateless: no framing,
  no partial reads to track across ticks.

## Consequences

* Not reachable from other hosts. That is a feature for now; a network
  export can be added later as a separate listener.
* Each `sysmonctl` call costs a connect(), fine at human speed.
* Responses are unstructured text: anything machine-oriented should get
  its own command rather than parse `list`.
