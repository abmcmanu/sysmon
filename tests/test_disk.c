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
    CHECK(is("/mnt/USB-Key", "mnt_usb_key"));
    CHECK(is("//", "root"));
    CHECK(is("/media/a.b", "media_a_b"));

    char small[8];
    disk_metric_name("/very/long/mount/point", small, sizeof small);
    CHECK(strlen(small) == 7);

    return test_report("disk");
}
