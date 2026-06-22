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
