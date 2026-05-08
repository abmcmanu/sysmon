#!/bin/sh
#
# release.sh X.Y.Z: bump the version, commit and tag.
# The CHANGELOG.md entry must be written first.
set -eu

v=${1:?usage: scripts/release.sh X.Y.Z}
case $v in
    *[!0-9.]* | .* | *.) echo "bad version: $v" >&2; exit 2 ;;
esac
[ -z "$(git status --porcelain)" ] || { echo "working tree not clean" >&2; exit 1; }
grep -q "^## $v " CHANGELOG.md || { echo "CHANGELOG.md has no entry for $v" >&2; exit 1; }

sed "s/^#define SYSMON_VERSION .*/#define SYSMON_VERSION \"$v\"/" src/version.h > src/version.h.new
mv src/version.h.new src/version.h
make clean all test
git commit -qam "Release $v"
git tag -a "v$v" -m "sysmon $v"
echo "tagged v$v; push with: git push origin main v$v"
