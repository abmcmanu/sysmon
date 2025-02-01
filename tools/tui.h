#ifndef SYSMON_TUI_H
#define SYSMON_TUI_H

/* `sysmonctl top`: full-screen dashboard, refreshed every interval_ms. */
int tui_run(const char *socket_path, int interval_ms);

#endif
