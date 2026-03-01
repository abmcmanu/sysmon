#!/bin/sh
#
# embed.sh NAME FILE: print FILE as a C byte array, for the http server.
# od is in POSIX, unlike xxd.
set -eu

name=$1
file=$2

printf '/* generated from %s, do not edit */\n' "$file"
printf 'static const unsigned char %s[] = {\n' "$name"
od -An -v -tx1 "$file" | awk '{ for (i = 1; i <= NF; i++) printf "0x%s,", $i; printf "\n" }'
printf '};\n'
printf 'static const unsigned int %s_len = sizeof %s;\n' "$name" "$name"
