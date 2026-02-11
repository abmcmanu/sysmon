# Configuration

`sysmond -c FILE` reads an ini-style file. `sysmond -t -c FILE` checks
it without starting.

```ini
# comment
; also a comment
[section]
key = value
```

* Keys before the first section belong to `[core]`.
* Unknown keys and invalid values are errors: the daemon refuses to start,
  and a reload keeps the previous configuration.
* `mount`, `load` and `rule` may be repeated; the others keep the last value.
