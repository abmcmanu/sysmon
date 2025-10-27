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

    sb_free(&out);
    return test_report("server");
}
