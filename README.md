# sysmon

`sysmond` is a small system monitoring daemon written in C. It samples
cpu, memory and load at a fixed interval and keeps them in memory.
`sysmonctl` queries it over a unix socket.

The original bash version (2013-2015) lives in [`legacy/`](legacy/).

## Build

Linux with any C11 compiler, no dependencies:

    make
    ./build/sysmond -f
    ./build/sysmonctl list

macOS is supported since 1.3 (Mach host statistics instead of `/proc`).

## Configuration

See [`contrib/sysmond.conf`](contrib/sysmond.conf) for every key.

    ./build/sysmond -c contrib/sysmond.conf

## Querying

    sysmonctl ping
    sysmonctl list
    sysmonctl get mem_used_percent
    sysmonctl history cpu_usage_percent 30

The protocol is plain text, see [docs/PROTOCOL.md](docs/PROTOCOL.md).

## License

MIT
