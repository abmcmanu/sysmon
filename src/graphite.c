#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "graphite.h"
#include "log.h"
#include "metrics.h"

static int fd = -1;
static char prefix[160];

void graphite_format(sb_t *out, const char *pfx, time_t ts)
{
    for (size_t i = 0; i < metrics_count(); i++) {
        const metric_t *m = metrics_at(i);
        if (!isnan(m->value))
            sb_printf(out, "%s.%s %.15g %lld\n", pfx, m->name, m->value, (long long)ts);
    }
}

int graphite_open(const config_t *cfg)
{
    struct addrinfo hints, *res;
    char service[8];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    snprintf(service, sizeof service, "%d", cfg->graphite_port);
    int rc = getaddrinfo(cfg->graphite_host, service, &hints, &res);
    if (rc != 0) {
        log_err("graphite: %s: %s", cfg->graphite_host, gai_strerror(rc));
        return -1;
    }
    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0 || connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
        log_err("graphite: %s:%d: %s", cfg->graphite_host, cfg->graphite_port, strerror(errno));
        if (fd >= 0)
            close(fd);
        fd = -1;
        freeaddrinfo(res);
        return -1;
    }
    freeaddrinfo(res);
    snprintf(prefix, sizeof prefix, "%s", cfg->graphite_prefix[0] ? cfg->graphite_prefix : "sysmon");
    log_info("graphite: sending to %s:%d as %s.*", cfg->graphite_host, cfg->graphite_port, prefix);
    return 0;
}

void graphite_send(void)
{
    sb_t out;

    if (fd < 0)
        return;
    sb_init(&out);
    graphite_format(&out, prefix, time(NULL));
    /* keep datagrams under a typical MTU, cut on line boundaries */
    size_t off = 0;
    while (off < out.len) {
        size_t n = out.len - off;
        if (n > 1400) {
            n = 1400;
            while (n > 0 && out.data[off + n - 1] != '\n')
                n--;
            if (n == 0)
                n = 1400;
        }
        if (send(fd, out.data + off, n, 0) < 0)
            log_debug("graphite: send: %s", strerror(errno));
        off += n;
    }
    sb_free(&out);
}

void graphite_close(void)
{
    if (fd >= 0)
        close(fd);
    fd = -1;
}
