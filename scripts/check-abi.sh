#!/bin/sh
#
# Fails when include/sysmon/plugin.h changed since the last release
# but SYSMON_PLUGIN_ABI did not.
set -eu

last=$(git describe --tags --abbrev=0)
if git diff --quiet "$last" -- include/sysmon/plugin.h; then
    echo "plugin.h unchanged since $last"
    exit 0
fi
old=$(git show "$last:include/sysmon/plugin.h" | sed -n 's/^#define SYSMON_PLUGIN_ABI //p')
new=$(sed -n 's/^#define SYSMON_PLUGIN_ABI //p' include/sysmon/plugin.h)
if [ "$old" = "$new" ]; then
    echo "plugin.h changed since $last but SYSMON_PLUGIN_ABI is still $new" >&2
    exit 1
fi
echo "plugin ABI $old -> $new since $last"
