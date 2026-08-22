#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "test.h"

int main(void)
{
    CHECK(log_level_from_str("error") == SM_LOG_ERR);
    CHECK(log_level_from_str("debug") == SM_LOG_DEBUG);
    CHECK(log_level_from_str("verbose") < 0);

    char path[] = "/tmp/sysmon-log-XXXXXX", line[256];
    int fd = mkstemp(path);
    close(fd);
    CHECK(log_open(path) == 0);
    log_set_level(SM_LOG_WARN);
    log_info("hidden");
    log_warn("shown %d", 42);
    log_close();

    FILE *f = fopen(path, "r");
    CHECK(f && fgets(line, sizeof line, f));
    CHECK(strstr(line, "[warn] shown 42") != NULL);
    CHECK(!fgets(line, sizeof line, f));
    fclose(f);
    unlink(path);
    CHECK(log_open("/nonexistent/dir/x.log") < 0);

    return test_report("log");
}
