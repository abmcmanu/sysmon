#!/usr/bin/env bash
#
# sysmon.sh - tiny system monitor for Linux boxes
#
# Reads /proc directly, no dependencies beyond bash and coreutils.
# Run `sysmon.sh -h` for options.

set -u

VERSION="0.1.0-dev"
INTERVAL=0

usage() {
    cat <<EOF
Usage: ${0##*/} [options]

  -i SEC   refresh every SEC seconds (default: print once and exit)
  -h       show this help
  -V       show version
EOF
}

# Prints "idle total" jiffies from the aggregated cpu line of /proc/stat.
read_cpu() {
    local cpu user nice system idle iowait irq softirq steal rest
    read -r cpu user nice system idle iowait irq softirq steal rest < /proc/stat
    echo "$idle $((user + nice + system + idle + iowait + irq + softirq + steal))"
}

# cpu usage in percent, measured over half a second
cpu_usage() {
    local i1 t1 i2 t2 dt di
    read -r i1 t1 <<< "$(read_cpu)"
    sleep 0.5
    read -r i2 t2 <<< "$(read_cpu)"
    dt=$((t2 - t1))
    di=$((i2 - i1))
    if [ "$dt" -le 0 ]; then
        echo 0
        return
    fi
    echo $(( (100 * (dt - di)) / dt ))
}

# prints "used_mb total_mb percent"
mem_usage() {
    local key val total=0 free=0 buffers=0 cached=0 avail=""
    while read -r key val _; do
        case $key in
            MemTotal:)     total=$val ;;
            MemFree:)      free=$val ;;
            Buffers:)      buffers=$val ;;
            Cached:)       cached=$val ;;
        esac
    done < /proc/meminfo
    avail=$((free + buffers + cached))
    local used=$((total - avail))
    echo "$((used / 1024)) $((total / 1024)) $((100 * used / total))"
}

render() {
    local cpu mem_used mem_total mem_pct l1 l5 l15
    cpu=$(cpu_usage)
    read -r mem_used mem_total mem_pct <<< "$(mem_usage)"
    read -r l1 l5 l15 _ < /proc/loadavg

    printf '%s  %s\n\n' "$(hostname)" "$(date '+%Y-%m-%d %H:%M:%S')"
    printf 'cpu   %3d%%\n' "$cpu"
    printf 'mem   %3d%%  %d / %d MB\n' "$mem_pct" "$mem_used" "$mem_total"
    printf 'load  %s %s %s\n' "$l1" "$l5" "$l15"
}

OPTS="i:hV"

while getopts "$OPTS" opt; do
    case $opt in
        i) INTERVAL=$OPTARG ;;
        h) usage; exit 0 ;;
        V) echo "sysmon $VERSION"; exit 0 ;;
        *) usage >&2; exit 2 ;;
    esac
done

if [ "$INTERVAL" = 0 ]; then
    render
    exit 0
fi

while :; do
    render
    sleep "$INTERVAL"
done
