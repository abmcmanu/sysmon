#!/bin/sh
# [alerts] exec hook: push notification through ntfy.sh (or your own server)
# NTFY_TOPIC=https://ntfy.sh/my-servers
set -eu
: "${NTFY_TOPIC:?set NTFY_TOPIC}"
prio=default
[ "$SYSMON_STATE" = firing ] && prio=high
curl -fsS -m 5 -H "Priority: $prio" -H "Title: $(hostname): $SYSMON_STATE" \
    -d "$SYSMON_ALERT (value $SYSMON_VALUE)" "$NTFY_TOPIC" > /dev/null
