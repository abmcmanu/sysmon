/*
 * sysmonctl - command line client for sysmond
 *
 *   sysmonctl list
 *   sysmonctl get cpu_usage_percent
 *   sysmonctl history mem_used_percent 30
 *   sysmonctl top
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "sb.h"
#include "version.h"
#include "client.h"
#include "tui.h"

#define DEFAULT_SOCKET "/tmp/sysmond.sock"

static void usage(FILE *out)
{
    fprintf(out,
            "usage: sysmonctl [-s SOCKET] [-w SEC] COMMAND [ARGS...]\n"
            "\n"
            "commands:\n"
            "  ping                   check that sysmond is alive\n"
            "  list [PATTERN]         metrics with their current value (glob: 'disk_*')\n"
            "  get METRIC             one metric\n"
            "  history METRIC [N]     the last N samples (default 60)\n"
            "  avg METRIC [N]         average of the last N samples\n"
            "  stats METRIC [N]       min, max, avg, p50, p95 of the last N samples\n"
            "  json                   all metrics as JSON\n"
            "  alerts                 alert rules and their state\n"
            "  reload                 re-read the configuration file\n"
            "  top [SEC]              interactive dashboard\n"
            "\n"
            "-w SEC repeats the command every SEC seconds, like watch(1).\n"
            "SOCKET defaults to $SYSMON_SOCKET, then " DEFAULT_SOCKET ".\n");
}

int main(int argc, char **argv)
{
    const char *sock = DEFAULT_SOCKET;
    int opt;
    double watch = 0;
    const char *env = getenv("SYSMON_SOCKET");

    if (env && *env)
        sock = env;

    while ((opt = getopt(argc, argv, "+s:w:hV")) != -1) {
        switch (opt) {
        case 's':
            sock = optarg;
            break;
        case 'w':
            watch = atof(optarg);
            if (watch < 0.1) {
                fprintf(stderr, "sysmonctl: -w needs at least 0.1 seconds\n");
                return 2;
            }
            break;
        case 'V':
            printf("sysmonctl %s\n", SYSMON_VERSION);
            {
                sb_t v;
                sb_init(&v);
                if (ctl_query(sock, "version", &v) == 0 && v.len)
                    fwrite(v.data, 1, v.len, stdout);
                sb_free(&v);
            }
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

    /* wait METRIC OP VALUE [TIMEOUT]: for scripts and CI jobs */
    if (strcmp(argv[optind], "wait") == 0) {
        if (argc - optind < 4) {
            fprintf(stderr, "usage: sysmonctl wait METRIC OP VALUE [TIMEOUT]\n");
            return 2;
        }
        const char *op = argv[optind + 2];
        double want = atof(argv[optind + 3]);
        char q[128];
        snprintf(q, sizeof q, "get %s", argv[optind + 1]);
        for (;;) {
            sb_t r;
            sb_init(&r);
            if (ctl_query(sock, q, &r) < 0 || !r.data || strncmp(r.data, "ERR", 3) == 0) {
                fprintf(stderr, "sysmonctl: %s", r.data ? r.data : "cannot reach sysmond\n");
                sb_free(&r);
                return 1;
            }
            double v = atof(r.data);
            sb_free(&r);
            if ((strcmp(op, ">") == 0 && v > want) || (strcmp(op, ">=") == 0 && v >= want) ||
                (strcmp(op, "<") == 0 && v < want) || (strcmp(op, "<=") == 0 && v <= want))
                return 0;
            sleep(1);
        }
    }

    if (strcmp(argv[optind], "top") == 0) {
        double secs = optind + 1 < argc ? atof(argv[optind + 1]) : 1.0;
        return tui_run(sock, (int)(secs * 1000.0)) < 0 ? 1 : 0;
    }

    sb_t cmd, out;
    sb_init(&cmd);
    sb_init(&out);
    for (int i = optind; i < argc; i++) {
        if (i > optind)
            sb_puts(&cmd, " ");
        sb_puts(&cmd, argv[i]);
    }

    while (watch > 0) {
        sb_reset(&out);
        if (ctl_query(sock, cmd.data, &out) < 0) {
            fprintf(stderr, "sysmonctl: cannot reach sysmond at %s: %s\n", sock, strerror(errno));
            return 3;
        }
        fputs("\033[H\033[2J", stdout);
        char ts[16];
        time_t t = time(NULL);
        strftime(ts, sizeof ts, "%H:%M:%S", localtime(&t));
        printf("every %gs: %s%*s\n\n", watch, cmd.data, 20, ts);
        if (out.len)
            fwrite(out.data, 1, out.len, stdout);
        fflush(stdout);
        struct timespec ts_sleep = { (time_t)watch, (long)((watch - (double)(time_t)watch) * 1e9) };
        nanosleep(&ts_sleep, NULL);
    }

    if (ctl_query(sock, cmd.data, &out) < 0) {
        fprintf(stderr, "sysmonctl: cannot reach sysmond at %s: %s\n", sock, strerror(errno));
        return 3;
    }
    if (out.len)
        fwrite(out.data, 1, out.len, stdout);
    int rc = out.len >= 3 && strncmp(out.data, "ERR", 3) == 0;
    sb_free(&cmd);
    sb_free(&out);
    return rc;
}
