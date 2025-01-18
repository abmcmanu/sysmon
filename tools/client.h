#ifndef SYSMON_CLIENT_H
#define SYSMON_CLIENT_H

#include "sb.h"

/* Send one command to sysmond and append the whole reply to `out`.
 * Returns -1 (errno set) if the daemon cannot be reached. */
int ctl_query(const char *socket_path, const char *cmd, sb_t *out);

#endif
