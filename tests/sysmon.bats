#!/usr/bin/env bats
#
# Run with: bats tests/   (https://github.com/sstephenson/bats)

SYSMON="$BATS_TEST_DIRNAME/../sysmon.sh"

@test "-V prints the version" {
    run "$SYSMON" -V
    [ "$status" -eq 0 ]
    [[ "$output" == sysmon\ * ]]
}

@test "-h prints usage" {
    run "$SYSMON" -h
    [ "$status" -eq 0 ]
    [[ "$output" == *Usage:* ]]
}

@test "one-shot run shows cpu, mem and load" {
    run "$SYSMON" -n
    [ "$status" -eq 0 ]
    [[ "$output" == *"cpu "* ]]
    [[ "$output" == *"mem "* ]]
    [[ "$output" == *"load "* ]]
}

@test "csv mode prints a header and one row" {
    run "$SYSMON" -C
    [ "$status" -eq 0 ]
    [ "${lines[0]}" = "ts,cpu,mem,load1,load5,load15" ]
    [[ "${lines[1]}" =~ ^[0-9]+,[0-9]+,[0-9]+, ]]
}

@test "unknown option fails with status 2" {
    run "$SYSMON" -Z
    [ "$status" -eq 2 ]
}

@test "-l appends a line to the log file" {
    log="$BATS_TMPDIR/sysmon-$$.log"
    rm -f "$log"
    run "$SYSMON" -n -l "$log"
    [ "$status" -eq 0 ]
    [ "$(wc -l < "$log")" -eq 1 ]
    rm -f "$log"
}
