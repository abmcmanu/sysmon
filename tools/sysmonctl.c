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

#include "sb.h"
#include "version.h"
#include "client.h"

#define DEFAULT_SOCKET "/tmp/sysmond.sock"

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
