#!/bin/sh
# [alerts] exec hook: append every state change to a file, one line each
exec >> "${SYSMON_HOOK_LOG:-/var/log/sysmon-alerts.log}"
printf '%s %s %s %s\n' "$(date '+%F %T')" "$SYSMON_STATE" "$SYSMON_VALUE" "$SYSMON_ALERT"
