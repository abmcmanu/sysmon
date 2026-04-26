#!/bin/sh
#
# run.sh [SECONDS]: run every fuzz target for SECONDS (default 60) each.
# Crashes are left in build-fuzz/ as crash-* files.
set -eu

secs=${1:-60}
cd "$(dirname "$0")/.."
make fuzz
for t in build-fuzz/fuzz/fuzz_*; do
    name=${t##*/}
    corpus=fuzz/corpus/${name#fuzz_}
    mkdir -p "build-fuzz/corpus/$name"
    echo "== $name, ${secs}s"
    "$t" -max_total_time="$secs" -artifact_prefix=build-fuzz/ "build-fuzz/corpus/$name" "$corpus"
done
