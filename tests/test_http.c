#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http.h"
#include "log.h"
#include "metrics.h"
#include "sb.h"
#include "test.h"

static int lfd;
static struct sockaddr_in addr;
static char reply[65536];

/* Send one request through a real socket and let http_handle() answer it. */
static const char *get(const char *request)
{
    int c = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(c, (struct sockaddr *)&addr, sizeof addr) < 0)
        return "";
    if (write(c, request, strlen(request)) < 0)
        return "";
    http_handle(lfd);

    size_t len = 0;
    ssize_t n;
    while (len < sizeof reply - 1 && (n = read(c, reply + len, sizeof reply - 1 - len)) > 0)
        len += (size_t)n;
    reply[len] = '\0';
    close(c);
    return reply;
}

int main(void)
{
    socklen_t alen = sizeof addr;

    log_set_level(-1);
    lfd = http_open("127.0.0.1", 0);
    CHECK(lfd >= 0);
    getsockname(lfd, (struct sockaddr *)&addr, &alen);
    CHECK(http_open("not-an-address", 0) < 0);
    CHECK(http_open("999.1.1.1", 0) < 0);
    metrics_set("load1", 0.5);

    CHECK(strncmp(get("GET /nope HTTP/1.0\r\n\r\n"), "HTTP/1.0 404", 12) == 0);
    CHECK(strstr(get("GET /metrics HTTP/1.0\r\n\r\n"), "\nsysmon_load1 0.5\n") != NULL);
    CHECK(strstr(reply, "sysmon_build_info{version=") != NULL);
    CHECK(strstr(get("GET /healthz HTTP/1.1\r\nHost: x\r\n\r\n"), "\r\n\r\nok\n") != NULL);
    CHECK(strstr(get("GET /api/metrics HTTP/1.0\r\n\r\n"), "\"load1\":0.5") != NULL);
    CHECK(strstr(reply, "Content-Type: application/json") != NULL);
    CHECK(strstr(reply, "Cache-Control: no-store") != NULL);
    metrics_set("load1", 1.5);
    CHECK(strstr(get("GET /api/history?metric=load1&n=2 HTTP/1.0\r\n\r\n"),
                 "{\"metric\":\"load1\",\"values\":[0.5,1.5]}") != NULL);
    CHECK(strncmp(get("GET /api/history?metric=zzz HTTP/1.0\r\n\r\n"), "HTTP/1.0 404", 12) == 0);
    CHECK(strncmp(get("POST /metrics HTTP/1.0\r\n\r\n"), "HTTP/1.0 405", 12) == 0);
    CHECK(strstr(get("GET /api/version HTTP/1.0\r\n\r\n"), "{\"version\":\"") != NULL);
    CHECK(strstr(get("GET /metrics?x=1 HTTP/1.0\r\n\r\n"), "sysmon_load1") != NULL);
    CHECK(strstr(get("GET /api/influx HTTP/1.0\r\n\r\n"), "\r\n\r\nsysmon,host=") != NULL);
    CHECK(strstr(reply, "load1=1.5") != NULL);

    close(lfd);
    return test_report("http");
}
