/*
 * sysmonctl - command line client for sysmond
 *
 *   sysmonctl list
 *   sysmonctl get cpu_usage_percent
 *   sysmonctl history mem_used_percent 30
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "sb.h"
#include "version.h"

#define DEFAULT_SOCKET "/tmp/sysmond.sock"

static int ctl_query(const char *path, const char *cmd, sb_t *out)
{
    struct sockaddr_un addr;

    if (strlen(path) >= sizeof addr.sun_path) {
        errno = ENAMETOOLONG;
        return -1;
    }
    memset(&addr, 0, sizeof addr);
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, path, strlen(path) + 1);

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;
    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
        int e = errno;
        close(fd);
        errno = e;
        return -1;
    }
    dprintf(fd, "%s\n", cmd);

    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof buf)) > 0)
        sb_append(out, buf, (size_t)n);
    close(fd);
    return 0;
}

static void usage(FILE *out)
{
    fprintf(out,
            "usage: sysmonctl [-s SOCKET] COMMAND [ARGS...]\n"
            "\n"
            "commands:\n"
            "  ping                   check that sysmond is alive\n"
            "  list                   all metrics with their current value\n"
            "  get METRIC             one metric\n"
            "  history METRIC [N]     the last N samples (default 60)\n"
            "  avg METRIC [N]         average of the last N samples\n"
            "  json                   all metrics as JSON\n"
            "  alerts                 alert rules and their state\n"
            "  reload                 re-read the configuration file\n"
            "\n"
            "SOCKET defaults to $SYSMON_SOCKET, then " DEFAULT_SOCKET ".\n");
}

int main(int argc, char **argv)
{
    const char *sock = DEFAULT_SOCKET;
    int opt;
    const char *env = getenv("SYSMON_SOCKET");

    if (env && *env)
        sock = env;

    while ((opt = getopt(argc, argv, "+s:hV")) != -1) {
        switch (opt) {
        case 's':
            sock = optarg;
            break;
        case 'V':
            printf("sysmonctl %s\n", SYSMON_VERSION);
            return 0;
        case 'h':
            usage(stdout);
            return 0;
        default:
            usage(stderr);
            return 2;
        }
    }
    if (optind >= argc) {
        usage(stderr);
        return 2;
    }

    sb_t cmd, out;
    sb_init(&cmd);
    sb_init(&out);
    for (int i = optind; i < argc; i++) {
        if (i > optind)
            sb_puts(&cmd, " ");
        sb_puts(&cmd, argv[i]);
    }

    if (ctl_query(sock, cmd.data, &out) < 0) {
        fprintf(stderr, "sysmonctl: cannot reach sysmond at %s: %s\n", sock, strerror(errno));
        return 1;
    }
    if (out.len)
        fwrite(out.data, 1, out.len, stdout);
    int rc = out.len >= 3 && strncmp(out.data, "ERR", 3) == 0;
    sb_free(&cmd);
    sb_free(&out);
    return rc;
}
