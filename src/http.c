#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "http.h"
#include "log.h"
#include "metrics.h"
#include "sb.h"
#include "version.h"
#include "json.h"

int http_open(const char *bind_addr, int port)
{
    struct sockaddr_in addr;
    int one = 1;

    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, bind_addr, &addr.sin_addr) != 1) {
        log_err("http: invalid bind address '%s'", bind_addr);
        return -1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        log_err("http: socket: %s", strerror(errno));
        return -1;
    }
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0 || listen(fd, 16) < 0) {
        log_err("http: %s:%d: %s", bind_addr, port, strerror(errno));
        close(fd);
        return -1;
    }
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    fcntl(fd, F_SETFD, FD_CLOEXEC);
    log_info("http: listening on %s:%d", bind_addr, port);
    return fd;
}

/* Prometheus text exposition format 0.0.4 */
static void render_prometheus(sb_t *out)
{
    sb_puts(out, "# HELP sysmon_build_info sysmond build information.\n"
                 "# TYPE sysmon_build_info gauge\n");
    sb_printf(out, "sysmon_build_info{version=\"%s\"} 1\n", SYSMON_VERSION);
    for (size_t i = 0; i < metrics_count(); i++) {
        const metric_t *m = metrics_at(i);
        if (m->help[0])
            sb_printf(out, "# HELP sysmon_%s %s\n", m->name, m->help);
        sb_printf(out, "# TYPE sysmon_%s gauge\n", m->name);
        if (isnan(m->value))
            sb_printf(out, "sysmon_%s NaN\n", m->name);
        else
            sb_printf(out, "sysmon_%s %.17g\n", m->name, m->value);
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

static const char *query_param(const char *query, const char *key, char *out, size_t size)
{
    size_t klen = strlen(key);

    for (const char *p = query; p && *p;) {
        if (strncmp(p, key, klen) == 0 && p[klen] == '=') {
            p += klen + 1;
            size_t n = strcspn(p, "&");
            if (n >= size)
                n = size - 1;
            memcpy(out, p, n);
            out[n] = '\0';
            return out;
        }
        p = strchr(p, '&');
        if (p)
            p++;
    }
    return NULL;
}

static const char *reason(int status)
{
    switch (status) {
    case 200: return "OK";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    default:  return "Error";
    }
}

void http_handle(int listen_fd)
{
    int fd = accept(listen_fd, NULL, NULL);
    if (fd < 0)
        return;
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK);
    struct timeval tv = { 1, 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);

    char req[2048];
    size_t len = 0;
    while (len < sizeof req - 1) {
        ssize_t n = read(fd, req + len, sizeof req - 1 - len);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            break;
        len += (size_t)n;
        req[len] = '\0';
        if (strstr(req, "\r\n\r\n") || strstr(req, "\n\n"))
            break;
    }
    req[len] = '\0';

    char method[8] = "", path[256] = "";
    if (sscanf(req, "%7s %255s", method, path) != 2) {
        close(fd);
        return;
    }
    char query[256] = "";
    char *q = strchr(path, '?');
    if (q)
        snprintf(query, sizeof query, "%s", q + 1);
    path[strcspn(path, "?")] = '\0';
    log_debug("http: %s %s", method, path);

    int status = 200;
    const char *ctype = "text/plain; charset=utf-8";
    const char *extra = strncmp(path, "/api/", 5) == 0 ? "Cache-Control: no-store\r\n" : "";
    sb_t body;
    sb_init(&body);

    if (strcmp(method, "GET") != 0) {
        status = 405;
        sb_puts(&body, "method not allowed\n");
    } else
    if (strcmp(path, "/") == 0) {
        ctype = "text/html; charset=utf-8";
        sb_printf(&body, "<html><head><title>sysmond</title></head><body>"
                         "<h1>sysmond %s</h1>"
                         "<p><a href=\"/metrics\">metrics</a></p>"
                         "</body></html>\n", SYSMON_VERSION);
    } else if (strcmp(path, "/api/metrics") == 0) {
        ctype = "application/json";
        json_metrics(&body);
    } else if (strcmp(path, "/api/history") == 0) {
        char metric[64], nbuf[16];
        const metric_t *m =
            query_param(query, "metric", metric, sizeof metric) ? metrics_get(metric) : NULL;
        ctype = "application/json";
        if (!m) {
            status = 404;
            sb_puts(&body, "{\"error\":\"unknown metric\"}\n");
        } else {
            long n = query_param(query, "n", nbuf, sizeof nbuf) ? strtol(nbuf, NULL, 10) : 60;
            json_history(&body, m, n > 0 ? (size_t)n : 60);
        }
    } else if (strcmp(path, "/api/version") == 0) {
        ctype = "application/json";
        sb_printf(&body, "{\"version\":\"%s\"}\n", SYSMON_VERSION);
    } else if (strcmp(path, "/metrics") == 0) {
        ctype = "text/plain; version=0.0.4; charset=utf-8";
        render_prometheus(&body);
    } else if (strcmp(path, "/healthz") == 0) {
        sb_puts(&body, "ok\n");
    } else {
        status = 404;
        sb_puts(&body, "not found\n");
    }

    char hdr[256];
    int n = snprintf(hdr, sizeof hdr,
                     "HTTP/1.0 %d %s\r\n"
                     "Content-Type: %s\r\n"
                     "%s"
                     "Content-Length: %zu\r\n"
                     "Connection: close\r\n\r\n",
                     status, reason(status), ctype, extra, body.len);
    if (write_all(fd, hdr, (size_t)n) == 0 && body.len)
        write_all(fd, body.data, body.len);
    sb_free(&body);
    close(fd);
}
