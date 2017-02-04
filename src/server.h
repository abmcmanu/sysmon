#ifndef SYSMON_SERVER_H
#define SYSMON_SERVER_H

/*
 * Control socket: one line-based command per connection.
 * See docs/PROTOCOL.md.
 */
int server_open(const char *path);
void server_handle(int listen_fd);
void server_close(int listen_fd, const char *path);

#endif
