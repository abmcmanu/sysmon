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
    CHECK(strcmp(run("ping\r\n"), "pong\n") == 0);
    CHECK(strcmp(run("  ping  "), "pong\n") == 0);

    sb_free(&out);
    return test_report("server");
}
