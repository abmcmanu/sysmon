#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

#include "log.h"
#include "metrics.h"
#include "sb.h"
#include "server.h"
#include "version.h"
#include "json.h"
#include "alert.h"

#define MAX_ARGS 4

static void (*reload_cb)(void);

void server_on_reload(void (*cb)(void))
{
    reload_cb = cb;
}

/* A connectable socket means another sysmond is alive: do not steal it. */
static int socket_in_use(const struct sockaddr_un *addr)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        return 0;
    int alive = connect(fd, (const struct sockaddr *)addr, sizeof *addr) == 0;
    close(fd);
    return alive;
}

int server_open(const char *path)
{
    struct sockaddr_un addr;

    if (strlen(path) >= sizeof addr.sun_path) {
        log_err("socket path too long: %s", path);
        return -1;
    }
    memset(&addr, 0, sizeof addr);
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, path, strlen(path) + 1);

    if (socket_in_use(&addr)) {
        log_err("%s is in use: is another sysmond running?", path);
        return -1;
    }
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        log_err("socket: %s", strerror(errno));
        return -1;
    }
    unlink(path); /* stale socket from a previous run */
    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0 || listen(fd, 16) < 0) {
        log_err("%s: %s", path, strerror(errno));
        close(fd);
        return -1;
    }
    chmod(path, 0660);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    fcntl(fd, F_SETFD, FD_CLOEXEC);
    log_info("listening on %s", path);
    return fd;
}

void server_close(int listen_fd, const char *path)
{
    if (listen_fd >= 0) {
        close(listen_fd);
        unlink(path);
    }
}

static int split(char *line, char **argv)
{
    int argc = 0;
    char *save = NULL;
    for (char *tok = strtok_r(line, " \t", &save); tok && argc < MAX_ARGS;
         tok = strtok_r(NULL, " \t", &save))
        argv[argc++] = tok;
    return argc;
}

static void cmd_list(sb_t *out)
{
    for (size_t i = 0; i < metrics_count(); i++) {
        const metric_t *m = metrics_at(i);
        sb_printf(out, "%s %.6g\n", m->name, m->value);
    }
}

static void cmd_history(sb_t *out, int argc, char **argv, int avg)
{
    if (argc < 2) {
        sb_printf(out, "ERR usage: %s METRIC [N]\n", argv[0]);
        return;
    }
    const metric_t *m = metrics_get(argv[1]);
    if (!m) {
        sb_printf(out, "ERR unknown metric '%s'\n", argv[1]);
        return;
    }
    long n = argc > 2 ? strtol(argv[2], NULL, 10) : 60;
    if (n <= 0)
        n = 60;
    if (avg) {
        sb_printf(out, "%.6g\n", ring_avg(&m->hist, (size_t)n));
        return;
    }
    size_t len = ring_len(&m->hist);
    size_t from = (size_t)n < len ? len - (size_t)n : 0;
    for (size_t i = from; i < len; i++)
        sb_printf(out, "%.6g\n", ring_at(&m->hist, i));
}

static void handle_cmd(char *line, sb_t *out)
{
    char *argv[MAX_ARGS];
    int argc = split(line, argv);

    if (argc == 0) {
        sb_puts(out, "ERR empty command\n");
    } else if (strcmp(argv[0], "ping") == 0) {
        sb_puts(out, "pong\n");
    } else if (strcmp(argv[0], "list") == 0) {
        cmd_list(out);
    } else if (strcmp(argv[0], "get") == 0) {
        const metric_t *m = argc > 1 ? metrics_get(argv[1]) : NULL;
        if (m)
            sb_printf(out, "%.6g\n", m->value);
        else
            sb_printf(out, "ERR unknown metric '%s'\n", argc > 1 ? argv[1] : "");
    } else if (strcmp(argv[0], "history") == 0) {
        cmd_history(out, argc, argv, 0);
    } else if (strcmp(argv[0], "avg") == 0) {
        cmd_history(out, argc, argv, 1);
    } else if (strcmp(argv[0], "json") == 0) {
        json_metrics(out);
    } else if (strcmp(argv[0], "alerts") == 0) {
        alerts_dump(out);
    } else if (strcmp(argv[0], "reload") == 0) {
        if (reload_cb) {
            reload_cb();
            sb_puts(out, "reloading\n");
        } else {
            sb_puts(out, "ERR reload not supported\n");
        }
    } else if (strcmp(argv[0], "version") == 0) {
        sb_printf(out, "sysmond %s\n", SYSMON_VERSION);
    } else if (strcmp(argv[0], "help") == 0) {
        sb_puts(out, "ping | version | list | get METRIC"
                     " | history METRIC [N] | avg METRIC [N]"
                     " | json"
                     " | alerts"
                     " | reload"
                     "\n");
    } else {
        sb_printf(out, "ERR unknown command '%s'\n", argv[0]);
    }
}

static int write_all(int fd, const char *p, size_t n)
{
    while (n > 0) {
        ssize_t w = write(fd, p, n);
        if (w < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        p += w;
        n -= (size_t)w;
    }
    return 0;
}

void server_handle(int listen_fd)
{
    int fd = accept(listen_fd, NULL, NULL);
    if (fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
            log_warn("accept: %s", strerror(errno));
        return;
    }
    /* BSDs (macOS included) let accepted sockets inherit O_NONBLOCK
     * from the listener; Linux does not. Be explicit. */
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK);
    /* a client that connects and never writes must not stall sampling */
    struct timeval tv = { 1, 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);

    char buf[512];
    size_t len = 0;
    while (len < sizeof buf - 1) {
        ssize_t n = read(fd, buf + len, sizeof buf - 1 - len);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            break;
        len += (size_t)n;
        if (memchr(buf, '\n', len))
            break;
    }
    buf[len] = '\0';
    buf[strcspn(buf, "\r\n")] = '\0';
    log_debug("ctl: '%s'", buf);

    sb_t out;
    sb_init(&out);
    handle_cmd(buf, &out);
    if (out.len && write_all(fd, out.data, out.len) < 0)
        log_debug("ctl: client went away: %s", strerror(errno));
    sb_free(&out);
    close(fd);
}
