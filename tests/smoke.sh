#!/bin/sh
#
# Smoke test: start a real sysmond in the foreground with a throwaway
# configuration, talk to it, stop it.
#
set -eu

BUILD=${BUILD:-build}
PORT=${SMOKE_HTTP_PORT:-19273}
tmp=$(mktemp -d "${TMPDIR:-/tmp}/sysmon-smoke.XXXXXX")
pid=""

cleanup() {
    [ -n "$pid" ] && kill "$pid" 2> /dev/null || true
    rm -rf "$tmp"
}
trap cleanup EXIT

fail() {
    echo "smoke: FAIL: $*" >&2
    echo "--- daemon log" >&2
    cat "$tmp/sysmond.log" >&2 || true
    exit 1
}

cat > "$tmp/sysmond.conf" << EOF
[core]
interval = 1
log_level = debug
log_file = $tmp/sysmond.log
socket = $tmp/sysmond.sock

[disk]
mount = /

[plugins]
dir = $BUILD/plugins
load = uptime
load = net

[http]
port = $PORT
EOF

"$BUILD/sysmond" -f -c "$tmp/sysmond.conf" &
pid=$!

ctl() { "$BUILD/sysmonctl" -s "$tmp/sysmond.sock" "$@"; }

i=0
until ctl ping > /dev/null 2>&1; do
    i=$((i + 1))
    [ "$i" -lt 50 ] || fail "daemon did not come up"
    sleep 0.1
done
sleep 2.5 # two samples, so cpu has a delta

ctl ping | grep -qx pong || fail "ping"
ctl list | grep -q '^cpu_usage_percent ' || fail "list: no cpu"
ctl list | grep -q '^disk_root_used_percent ' || fail "list: no disk"
ctl list | grep -q '^uptime_seconds ' || fail "list: no plugin metric"
ctl get mem_used_percent > /dev/null || fail "get"
! ctl get no_such_metric > /dev/null 2>&1 || fail "get: unknown metric accepted"
[ "$(ctl history load1 2 | wc -l)" -eq 2 ] || fail "history"

if command -v curl > /dev/null 2>&1; then
    curl -fsS "http://127.0.0.1:$PORT/metrics" | grep -q '^sysmon_cpu_usage_percent ' ||
        fail "http /metrics"
    curl -fsS "http://127.0.0.1:$PORT/healthz" | grep -qx ok || fail "http /healthz"
else
    echo "smoke: curl not found, skipping http checks"
fi

kill "$pid"
wait "$pid" || fail "daemon exited with an error"
pid=""
[ ! -e "$tmp/sysmond.sock" ] || fail "socket not removed on exit"
echo "smoke: OK"
