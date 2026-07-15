#!/bin/sh
# [alerts] exec hook: Slack incoming webhook
# SLACK_WEBHOOK=https://hooks.slack.com/services/...
set -eu
: "${SLACK_WEBHOOK:?set SLACK_WEBHOOK}"
icon=":white_check_mark:"
[ "$SYSMON_STATE" = firing ] && icon=":rotating_light:"
text="$icon *$(hostname)* $SYSMON_ALERT is $SYSMON_STATE (value $SYSMON_VALUE)"
curl -fsS -m 5 -H 'Content-Type: application/json' \
    -d "{\"text\": \"$text\"}" "$SLACK_WEBHOOK" > /dev/null
