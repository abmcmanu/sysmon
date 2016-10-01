# 1. Rewrite sysmon as a C daemon

* Status: accepted (2016-10, shipped in 1.0.0)
* Date: 2015-11-28

## Context

`sysmon.sh` has been useful for two and a half years but it hit a wall:

* Every refresh forks ~15 processes (`date`, `ps`, `df`, `awk`, subshells).
  On a small VPS with `-i 1` the monitor itself shows up in the top
  processes, which is embarrassing for a monitor.
* There is no memory between two refreshes: no history, no averages,
  "for N minutes" alerts are impossible without a state file.
* Nothing else can consume the numbers except by scraping the output.
* bash arithmetic is integer only.

## Decision

Rewrite the core as a long-running daemon in C (C11, POSIX, no external
dependencies) that samples at a fixed interval and keeps the values in
memory. Display and integrations become clients of that daemon.

C rather than Go or Python because the daemon must be tiny (a few
hundred KB of RSS), start instantly, build on any box with a compiler
and read `/proc` with nothing in between.

## Consequences

* Two programs instead of one script: the daemon and a client.
* Memory safety becomes our problem: keep the code small, no dynamic
  allocation in the sampling path, add sanitizers to the build.
* The shell version stays in `legacy/` for machines without a compiler.
