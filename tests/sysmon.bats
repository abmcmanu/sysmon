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
