# sysmon

A tiny system monitor for Linux, in a single bash script. No
dependencies: it reads `/proc` directly.

```
$ ./sysmon.sh
web01  2013-04-06 22:41:07

cpu    12%  [###...........................]
mem    41%  [############..................]  1673 / 3953 MB
load  0.21 0.18 0.12
```

## Usage

```
sysmon.sh              # print once and exit
sysmon.sh -i 2         # refresh every 2 seconds, Ctrl-C to quit
```

Install it somewhere in your `$PATH`:

    cp sysmon.sh ~/bin/sysmon

Besides cpu/memory/load it shows filesystem usage
for every mounted filesystem.

## License

MIT
