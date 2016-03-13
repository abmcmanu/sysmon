/*
 * sysmond - small system monitoring daemon
 *
 * Samples cpu, memory and load at a fixed interval and keeps the
 * latest values in an in-memory registry (metrics.c).
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

#include "collector.h"
#include "config.h"
#include "log.h"
#include "metrics.h"
#include "version.h"

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

static void tick(const config_t *cfg)
{
    collect_sample();
    print_sample();
    (void)cfg;
}

int main(int argc, char **argv)
{
    config_t cfg;
    const char *cfg_path = NULL;
    int foreground = 0;
    int interval = 0, level = -1, opt;
    static const char optstring[] = "hVi:l:"
                                    "c:"
                                    "f"
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
    log_set_level(cfg.log_level);
    if (cfg.log_file[0] && log_open(cfg.log_file) < 0)
        return 1;

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_signal; /* no SA_RESTART: we want EINTR */
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if (collect_init() < 0)
        return 1;

    /* Open listeners and plugins before daemonizing: errors still reach
     * the terminal and relative paths still mean something. */

    if (!foreground && daemonize() < 0) {
        log_err("daemonize: %s", strerror(errno));
        return 1;
    }
    if (!foreground && cfg.pid_file[0] && write_pidfile(cfg.pid_file) < 0)
        return 1;

    log_info("sysmond %s started (pid %ld, interval %ds)", SYSMON_VERSION, (long)getpid(),
             cfg.interval);

    while (running) {
        tick(&cfg);
        sleep((unsigned)cfg.interval);
    }

    log_info("shutting down");
    if (!foreground && cfg.pid_file[0])
        unlink(cfg.pid_file);
    log_close();
    return 0;
}
