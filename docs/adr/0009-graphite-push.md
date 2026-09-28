# 9. Push to Graphite over UDP

* Status: accepted
* Date: 2026-08

## Context

ADR 0005 chose pull (Prometheus) and left push for later. Several
setups still run Graphite/carbon, often behind NAT where nothing can
scrape the hosts.

## Decision

A built-in, optional sender for the carbon plaintext protocol over UDP,
fired at the end of every tick. Not a plugin: plugins cannot read the
registry, and giving them that would be a much bigger ABI change than
this feature is worth.

UDP because the daemon must never block on the network: a send either
goes into the kernel buffer or is dropped.

## Consequences

* Lost datagrams are gaps in the graphs; acceptable for monitoring.
* No TCP, no batching beyond one tick, no TLS. Use a local relay
  (carbon-relay, Telegraf) for anything fancier.
