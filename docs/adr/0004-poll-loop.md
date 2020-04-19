# 4. Single-threaded poll() loop

* Status: accepted
* Date: 2020-04-19

## Context

Since 1.1 the main loop waits for the next tick with `select()` on the
control socket. More listeners are coming (an HTTP exporter is planned)
and `select()` caps descriptors at `FD_SETSIZE`.

Threads were considered: one sampler, one per listener. They would need
locking around the metric registry, and every plugin would suddenly have
to be thread safe.

## Decision

Keep one thread. The loop is:

    loop:
        sample everything            (collectors, plugins, alerts)
        next += interval
        until next:
            poll(listeners, timeout = next - now)
            handle each ready listener, one request each

Listeners are a small table of `{fd, handler}`; adding the HTTP server
is one `add_listener()` call. Handlers must not block: client sockets
get a 1 s receive timeout.

## Consequences

* No locks anywhere; plugins and collectors stay simple.
* A slow request delays the next sample by at most the client timeout.
  The loop never tries to "catch up" missed ticks (e.g. after a
  suspend): it resynchronizes on the current time.
* If one day sampling becomes expensive, it is the part to move to a
  thread, with a snapshot handed over to the loop.
