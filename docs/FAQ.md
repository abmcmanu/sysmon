# FAQ

### Why not node_exporter / collectd / Netdata?

They are great and do much more. sysmon is for when you want one small
binary with no dependencies, a CLI you can script, and a history you
can query without a database. It also runs on a Raspberry Pi Zero
without anyone noticing.

### Why is cpu_usage_percent missing right after start?

It is a difference between two samples. The first value appears after
one interval.

### `mem_used_percent` does not match `free` / Activity Monitor

On Linux it is `MemTotal - MemAvailable`, which counts reclaimable
cache as free, like the "available" column of `free`. On macOS it is
app memory + wired + compressed, like Activity Monitor's "Memory Used".

### Can I get more than 300 samples of history?

Not in sysmond: the ring buffer size is fixed at compile time
(`RING_CAP` in `src/ring.h`) to keep memory bounded. Long term storage
is Prometheus' job.

### sysmonctl says "cannot reach sysmond"

Check which socket the daemon uses (`socket` in the config, default
`/tmp/sysmond.sock`) and that you can write to it. Under systemd with
`DynamicUser=yes`, `/tmp` is private: use `/run/sysmond/sysmond.sock`.

### Does it work in a container?

Yes, see [`contrib/docker/`](../contrib/docker/). To monitor the host,
run it with `--pid=host --network=host` and mount the host's root file
system for disk usage.
