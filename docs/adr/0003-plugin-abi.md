# 3. Plugins as shared objects with a versioned host vtable

* Status: accepted
* Date: 2019-03-16

## Context

Network, temperature, GPU, application-specific counters... every
metric source added to the core makes it bigger and more platform
specific. We want third parties (and us) to add collectors without
touching the daemon.

## Decision

Plugins are shared objects loaded with `dlopen()` from `[plugins] dir`.
Each exports a single function, `sysmon_plugin_entry()`, returning a
`sysmon_plugin_t` (name, version, `init`, `collect`, `fini`).

The daemon passes a `sysmon_host_t` to `init()`: a table of function
pointers (`set`, `set_help`, `log`). Plugins never call daemon symbols
directly and are linked without it.

Both structs carry an `abi` integer, `SYSMON_PLUGIN_ABI`. The daemon
refuses plugins built for another ABI.

The whole contract is one header, `include/sysmon/plugin.h`.

## Alternatives considered

* **Exec'd scripts** printing `name value` lines (like Munin/collectd
  exec): simple and language agnostic, but a fork per plugin per tick
  is exactly what we left bash to avoid.
* **Linking against libsysmon**: plugins would depend on internal
  symbols and break on every refactor.

## Consequences

* A crashing plugin crashes the daemon. Mitigation: plugins are
  disabled after 5 consecutive `collect()` failures; anything risky
  should stay out of process.
* Adding a field to the host table is backward compatible only if it is
  appended; anything else bumps the ABI.
