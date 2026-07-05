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
