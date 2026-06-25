# Contributing

Bug reports and patches are welcome. For anything bigger than a fix,
open an issue first so we can agree on the approach.

## Building and testing

```sh
make            # daemon, client, plugins
make test       # unit tests
make smoke      # end-to-end: real daemon, real socket, real HTTP
make asan       # all of the above under AddressSanitizer + UBSan
```

Everything must pass on Linux and macOS; CI also builds on FreeBSD and
Alpine (musl).

## Code style

* C11 + POSIX, no other dependency. If you need a library, it is
  probably a plugin.
* 4 spaces, 100 columns, braces as in the rest of the code;
  `.clang-format` describes it.
* No allocation in the sampling path. Fixed-size buffers, checked.
* Comments say *why*. The code says what.
* Every new behaviour comes with a test in `tests/`.

## Commits

* One logical change per commit, the tree builds and tests pass at
  every commit.
* Subject line: `area: what changed`, lower case after the colon, no
  period, under 72 characters. `server: refuse requests longer than 512 bytes`.
* The body, if any, explains why.

## Plugins and the ABI

`include/sysmon/plugin.h` is a public interface. Fields are only ever
appended; any change bumps `SYSMON_PLUGIN_ABI` and needs an ADR in
`docs/adr/`. `scripts/check-abi.sh` catches forgotten bumps in CI.

## Releasing

1. Write the `CHANGELOG.md` entry.
2. `scripts/release.sh X.Y.Z`
3. `git push origin main vX.Y.Z`; the release workflow does the rest.
