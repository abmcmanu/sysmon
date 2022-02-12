/*
 * sysmond - small system monitoring daemon
 *
 * Samples cpu, memory and load at a fixed interval and keeps the
 * latest values in an in-memory registry (metrics.c).
 * Clients query it over a unix socket (docs/PROTOCOL.md).
 * Extra metrics come from dlopen()ed plugins (docs/PLUGINS.md).
 * Prometheus scrapes it over HTTP at /metrics.
 *
 * Single thread, single poll() loop: see docs/adr/0004-poll-loop.md.
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <poll.h>

#include "collector.h"
#include "config.h"
#include "log.h"
#include "metrics.h"
#include "version.h"
#include "server.h"
#include "plugins.h"
#include "http.h"
#include "alert.h"

static volatile sig_atomic_t running = 1;

static void on_signal(int sig)
{
    (void)sig;
    running = 0;
}

static void usage(FILE *out)
{
    fprintf(out,
            "usage: sysmond [options]\n"
            "\n"
            "  -c FILE   configuration file\n"
            "  -f        stay in the foreground\n"
            "  -i SEC    sampling interval in seconds (default: 5)\n"
            "  -l LEVEL  log level: error, warn, info, debug\n"
            "  -p        print every sample to stdout\n"
            "  -V        print version and exit\n"
            "  -h        show this help\n");
}

static void print_sample(void)
{
    char ts[32];
    time_t t = time(NULL);
    struct tm tm;

    localtime_r(&t, &tm);
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", &tm);
    printf("%s", ts);
    for (size_t i = 0; i < metrics_count(); i++) {
        const metric_t *m = metrics_at(i);
        printf(" %s=%.2f", m->name, m->value);
    }
    putchar('\n');
    fflush(stdout);
}

static int daemonize(void)
{
    pid_t pid = fork();
    if (pid < 0)
        return -1;
    if (pid > 0)
        _exit(0);
    if (setsid() < 0)
        return -1;
    pid = fork(); /* never reacquire a controlling terminal */
    if (pid < 0)
        return -1;
    if (pid > 0)
        _exit(0);

    umask(022);
    if (chdir("/") < 0)
        return -1;
    int fd = open("/dev/null", O_RDWR);
    if (fd >= 0) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO)
            close(fd);
    }
    return 0;
}

static int write_pidfile(const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f) {
        log_err("pid file %s: %s", path, strerror(errno));
        return -1;
    }
    fprintf(f, "%ld\n", (long)getpid());
    fclose(f);
    return 0;
}

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

#define MAX_LISTENERS 4

typedef struct listener {
    int fd;
    void (*handle)(int fd);
} listener_t;

static listener_t listeners[MAX_LISTENERS];
static int nlisteners;

static void add_listener(int fd, void (*handle)(int))
{
    listeners[nlisteners].fd = fd;
    listeners[nlisteners].handle = handle;
    nlisteners++;
}

/* Serve clients until `deadline`, the time of the next sample. */
static void wait_until(double deadline)
{
    struct pollfd pfds[MAX_LISTENERS];

    while (running) {
        double d = deadline - now();
        if (d <= 0)
            return;
        for (int i = 0; i < nlisteners; i++) {
            pfds[i].fd = listeners[i].fd;
            pfds[i].events = POLLIN;
            pfds[i].revents = 0;
        }
        int n = poll(pfds, (nfds_t)nlisteners, (int)(d * 1000.0) + 1);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            log_err("poll: %s", strerror(errno));
            return;
        }
        for (int i = 0; i < nlisteners; i++)
            if (pfds[i].revents & POLLIN)
                listeners[i].handle(listeners[i].fd);
    }
}

static void tick(const config_t *cfg)
{
    collect_sample();
    collect_disks(cfg);
    plugins_collect();
    alerts_eval();
    if (cfg->print)
        print_sample();
}

int main(int argc, char **argv)
{
    config_t cfg;
    const char *cfg_path = NULL;
    int foreground = 0;
    int print = 0;
    int interval = 0, level = -1, opt;
    static const char optstring[] = "hVi:l:"
                                    "c:"
                                    "f"
                                    "p"
                                    ;

    config_defaults(&cfg);
    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
        case 'c':
            cfg_path = optarg;
            break;
        case 'f':
            foreground = 1;
            break;
        case 'p':
            print = 1;
            break;
        case 'i':
            interval = atoi(optarg);
            if (interval < 1) {
                fprintf(stderr, "sysmond: invalid interval '%s'\n", optarg);
                return 2;
            }
            break;
        case 'l':
            level = log_level_from_str(optarg);
            if (level < 0) {
                fprintf(stderr, "sysmond: invalid log level '%s'\n", optarg);
                return 2;
            }
            break;
        case 'V':
            printf("sysmond %s\n", SYSMON_VERSION);
            return 0;
        case 'h':
            usage(stdout);
            return 0;
        default:
            usage(stderr);
            return 2;
        }
    }

    if (cfg_path && config_load(&cfg, cfg_path) < 0)
        return 1;
    /* the command line wins over the configuration file */
    if (interval)
        cfg.interval = interval;
    if (level >= 0)
        cfg.log_level = level;
    cfg.print = print;
    log_set_level(cfg.log_level);
    if (cfg.log_file[0] && log_open(cfg.log_file) < 0)
        return 1;

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_signal; /* no SA_RESTART: we want EINTR */
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN); /* a client hanging up must not kill us */

    if (collect_init() < 0)
        return 1;
    if (alerts_load(cfg.alerts, cfg.nalerts) < 0)
        return 1;

    /* Open listeners and plugins before daemonizing: errors still reach
     * the terminal and relative paths still mean something. */
    int srv = server_open(cfg.socket_path);
    if (srv < 0)
        return 1;
    add_listener(srv, server_handle);
    int http_fd = -1;
    if (cfg.http_port > 0) {
        http_fd = http_open(cfg.http_bind, cfg.http_port);
        if (http_fd < 0)
            return 1;
        add_listener(http_fd, http_handle);
    }
    plugins_load(&cfg);

    if (!foreground && daemonize() < 0) {
        log_err("daemonize: %s", strerror(errno));
        return 1;
    }
    if (!foreground && cfg.pid_file[0] && write_pidfile(cfg.pid_file) < 0)
        return 1;

    log_info("sysmond %s started (pid %ld, interval %ds)", SYSMON_VERSION, (long)getpid(),
             cfg.interval);

    double next = now();
    while (running) {
        tick(&cfg);
        next += cfg.interval;
        double t = now();
        if (next < t) /* overran, or the machine was suspended: do not catch up */
            next = t;
        wait_until(next);
    }

    log_info("shutting down");
    plugins_unload();
    if (http_fd >= 0)
        close(http_fd);
    server_close(srv, cfg.socket_path);
    if (!foreground && cfg.pid_file[0])
        unlink(cfg.pid_file);
    log_close();
    return 0;
}
