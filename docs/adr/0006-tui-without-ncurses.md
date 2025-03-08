# 6. Terminal UI with raw ANSI sequences, no ncurses

* Status: accepted
* Date: 2025-03-08

## Context

`sysmonctl list` is fine for scripts but people keep running
`watch -n1 sysmonctl list`. We want a `top`-like view.

ncurses is the obvious choice, but it is the first non-libc dependency
of the project, its headers are missing on minimal images, and macOS
ships an old version with its own quirks.

## Decision

Implement `sysmonctl top` with termios (raw mode) and a handful of
VT100/xterm escape sequences: alternate screen, cursor hide, clear,
SGR colors. The whole screen is built in a buffer and written with one
`write()` per refresh, which is enough to avoid flicker. Sparklines use
the Unicode block elements U+2581..U+2588.

## Consequences

* Zero new dependencies; `sysmonctl` still builds with just a C compiler.
* Requires a UTF-8, ANSI-capable terminal (every terminal in use today).
* No incremental redraw: the screen is repainted every refresh. At one
  refresh per second this is a few KB, negligible even over SSH.
