#include <string.h>

#include "metrics.h"
#include "sb.h"
#include "server.h"
#include "test.h"

static sb_t out;

static const char *run(const char *cmd)
{
    sb_reset(&out);
    server_exec(cmd, &out);
    return out.data ? out.data : "";
}

int main(void)
{
    sb_init(&out);
    CHECK(strcmp(run("ping"), "pong\n") == 0);
    CHECK(strncmp(run(""), "ERR", 3) == 0);
    CHECK(strncmp(run("nope"), "ERR unknown command", 19) == 0);

    metrics_set("load1", 0.5);
    metrics_set("mem_used_bytes", 12345678901.0);
    CHECK(strcmp(run("get load1"), "0.5\n") == 0);
    CHECK(strncmp(run("get missing"), "ERR", 3) == 0);
    CHECK(strstr(run("list"), "load1 0.5\n") != NULL);
    CHECK(strcmp(run("get mem_used_bytes"), "12345678901\n") == 0);

    metrics_set("load1", 1.5);
    metrics_set("load1", 2.5);
    CHECK(strcmp(run("history load1 2"), "1.5\n2.5\n") == 0);
    CHECK(strcmp(run("avg load1 2"), "2\n") == 0);
    CHECK(strncmp(run("history"), "ERR usage", 9) == 0);

    CHECK(strstr(run("help"), "history METRIC") != NULL);
    CHECK(strncmp(run("version"), "sysmond ", 8) == 0);
    CHECK(strcmp(run("get"), "ERR usage: get METRIC\n") == 0);

    metrics_set("s", 10);
    metrics_set("s", 20);
    metrics_set("s", 30);
    CHECK(strcmp(run("stats s 3"), "min 10\nmax 30\navg 20\np50 20\np95 30\nlast 30\n") == 0);
    CHECK(strncmp(run("stats"), "ERR usage", 9) == 0);

    CHECK(strstr(run("list load*"), "load1 ") != NULL);
    CHECK(strstr(run("list load*"), "mem_used_bytes") == NULL);
    CHECK(strcmp(run("list nothing_matches_*"), "") == 0);

    char big[600];
    memset(big, 'x', sizeof big - 1);
    big[sizeof big - 1] = '\0';
    CHECK(strcmp(run(big), "ERR request too long\n") == 0);
    CHECK(strcmp(run("ping\r\n"), "pong\n") == 0);
    CHECK(strcmp(run("  ping  "), "pong\n") == 0);

    sb_free(&out);
    return test_report("server");
}
