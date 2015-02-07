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
sysmon.sh -i 5 -c 80   # warn when cpu >= 80%
sysmon.sh -i 60 -l /var/log/sysmon.csv   # keep a log
sysmon.sh -i 10 -C > load.csv            # csv for later
```

Install it somewhere in your `$PATH`:

    cp sysmon.sh ~/bin/sysmon

Besides cpu/memory/load it shows filesystem usage
and the five busiest processes.

## Alerts

With `-c` and `-m`, a line `!! ALERT: ...` is printed on stderr when cpu
or memory usage reaches the threshold (90% by default).
Alerts also go to syslog (`logger -t sysmon`), so they end up in
`/var/log/syslog` or `journalctl -t sysmon`.

## Plotting

`-C` prints `ts,cpu,mem,load1,load5,load15` lines. One hour of data,
plotted with gnuplot:

```sh
./sysmon.sh -i 10 -C > /tmp/sysmon.csv &   # let it run for a while
gnuplot -p -e "set datafile separator ','; set xdata time; set timefmt '%s'; \
  plot '/tmp/sysmon.csv' using 1:2 with lines title 'cpu', '' using 1:3 with lines title 'mem'"
```

## Tests

    make test     # needs bats: https://github.com/sstephenson/bats

## License

MIT
