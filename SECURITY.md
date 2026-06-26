# Security

Report vulnerabilities through GitHub: open a private security advisory
(Security tab, "Report a vulnerability"), or an issue if you are unsure
it is one. You will get an answer within a week.

## Attack surface

* The control socket: anyone who can connect can read every metric and
  trigger a reload. Its permissions (`socket_mode`, default `0660`) are
  the access control.
* The HTTP listener, when enabled: read-only, unauthenticated. It binds
  to 127.0.0.1 unless told otherwise.
* Plugins run inside the daemon with its privileges. Only load plugins
  you trust, from a directory only root can write.
* Alert hooks run with the daemon's privileges and environment.

sysmond never needs root once started: set `[core] user`.
