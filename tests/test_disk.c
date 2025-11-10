#include <string.h>

#include "collector.h"
#include "test.h"

static int is(const char *mnt, const char *want)
{
    char b[48];
    disk_metric_name(mnt, b, sizeof b);
    return strcmp(b, want) == 0;
}

int main(void)
{
    CHECK(is("/", "root"));
    CHECK(is("/home", "home"));
    CHECK(is("/var/lib/docker", "var_lib_docker"));

    return test_report("disk");
}
