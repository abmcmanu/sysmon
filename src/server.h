#ifndef SYSMON_SERVER_H
#define SYSMON_SERVER_H

/*
 * Control socket: one line-based command per connection.
 * See docs/PROTOCOL.md.
 */
#include "sb.h"

int server_open(const char *path, int mode);
void server_handle(int listen_fd);
void server_close(int listen_fd, const char *path);
/* Run one command and append the reply to `out`. */
void server_exec(const char *line, sb_t *out);

/* Called when a client sends `reload`. */
void server_on_reload(void (*cb)(void));

#endif
