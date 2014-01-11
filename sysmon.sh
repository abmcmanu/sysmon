#!/usr/bin/env bash
#
# sysmon.sh - tiny system monitor for Linux boxes
#
# Reads /proc directly, no dependencies beyond bash and coreutils.
# Run `sysmon.sh -h` for options.

set -u

VERSION="0.2.0"
INTERVAL=0
USE_COLOR=1
CPU_MAX=90
MEM_MAX=90
LOGFILE=""

usage() {
    cat <<EOF
Usage: ${0##*/} [options]

  -i SEC   refresh every SEC seconds (default: print once and exit)
  -c PCT   cpu alert threshold in percent (default: $CPU_MAX)
  -m PCT   memory alert threshold in percent (default: $MEM_MAX)
  -l FILE  append one line per sample to FILE
  -n       no colors
  -h       show this help
  -V       show version
EOF
}

# Prints "idle total" jiffies from the aggregated cpu line of /proc/stat.
read_cpu() {
    local cpu user nice system idle iowait irq softirq steal rest
    read -r cpu user nice system idle iowait irq softirq steal rest < /proc/stat
    # iowait is time spent idle while waiting for I/O: count it as idle
    echo "$((idle + iowait)) $((user + nice + system + idle + iowait + irq + softirq + steal))"
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

setup_colors() {
    if [ "$USE_COLOR" -eq 1 ] && [ -t 1 ]; then
        RED=$'\e[31m'
        YELLOW=$'\e[33m'
        GREEN=$'\e[32m'
        BOLD=$'\e[1m'
        RESET=$'\e[0m'
    else
        RED="" YELLOW="" GREEN="" BOLD="" RESET=""
    fi
}

level_color() {
    if [ "$1" -ge 90 ]; then
        printf '%s' "$RED"
    elif [ "$1" -ge 70 ]; then
        printf '%s' "$YELLOW"
    else
        printf '%s' "$GREEN"
    fi
}

# bar PCT -> [#########...........]
bar() {
    local pct=$1 width=30 n i out=""
    [ "$pct" -gt 100 ] && pct=100
    n=$((pct * width / 100))
    for ((i = 0; i < width; i++)); do
        if [ "$i" -lt "$n" ]; then out+="#"; else out+="."; fi
    done
    printf '[%s%s%s]' "$(level_color "$pct")" "$out" "$RESET"
}

check_alerts() {
    local cpu=$1 mem=$2 msg=""
    [ "$cpu" -ge "$CPU_MAX" ] && msg+="cpu ${cpu}% >= ${CPU_MAX}% "
    [ "$mem" -ge "$MEM_MAX" ] && msg+="mem ${mem}% >= ${MEM_MAX}% "
    [ -z "$msg" ] && return 0
    printf '%s!! ALERT: %s%s\n' "$RED" "$msg" "$RESET" >&2
}

render() {
    local cpu mem_used mem_total mem_pct l1 l5 l15
    cpu=$(cpu_usage)
    read -r mem_used mem_total mem_pct <<< "$(mem_usage)"
    read -r l1 l5 l15 _ < /proc/loadavg

    printf '%s%s%s  %s\n\n' "$BOLD" "$(hostname)" "$RESET" "$(date '+%Y-%m-%d %H:%M:%S')"
    printf 'cpu   %3d%%  %s\n' "$cpu" "$(bar "$cpu")"
    printf 'mem   %3d%%  %s  %d / %d MB\n' "$mem_pct" "$(bar "$mem_pct")" "$mem_used" "$mem_total"
    printf 'load  %s %s %s\n' "$l1" "$l5" "$l15"

    echo "disks"
    df -P -x tmpfs -x devtmpfs 2>/dev/null | awk 'NR>1 {gsub("%","",$5); printf "  %-24s %3d%%\n", $6, $5}'

    echo "top processes"
    ps -eo pid,pcpu,pmem,comm --sort=-pcpu | head -n 6 | sed 's/^/  /'

    check_alerts "$cpu" "$mem_pct"
    [ -n "$LOGFILE" ] && echo "$(date '+%F %T'),$cpu,$mem_pct,$l1" >> $LOGFILE
}

OPTS="i:hV"
OPTS+="c:m:"
OPTS+="l:"
OPTS+="n"

while getopts "$OPTS" opt; do
    case $opt in
        i) INTERVAL=$OPTARG ;;
        c) CPU_MAX=$OPTARG ;;
        m) MEM_MAX=$OPTARG ;;
        l) LOGFILE=$OPTARG ;;
        n) USE_COLOR=0 ;;
        h) usage; exit 0 ;;
        V) echo "sysmon $VERSION"; exit 0 ;;
        *) usage >&2; exit 2 ;;
    esac
done

if [ ! -r /proc/stat ]; then
    echo "${0##*/}: /proc/stat is not readable (this script only works on Linux)" >&2
    exit 1
fi

setup_colors

if [ "$INTERVAL" = 0 ]; then
    render
    exit 0
fi

trap 'printf "\e[?25h"; exit 0' INT TERM
printf '\e[?25l'

while :; do
    clear
    render
    sleep "$INTERVAL"
done
