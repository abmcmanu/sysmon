#include <string.h>

#include "graphite.h"
#include "metrics.h"
#include "test.h"

int main(void)
{
    sb_t o;

    sb_init(&o);
    metrics_set("load1", 0.25);
    metrics_set_help("not_sampled_yet", "x");
    metrics_set("mem_used_bytes", 12345678901.0);
    graphite_format(&o, "sysmon.web01", 1700000000);
    CHECK(strcmp(o.data, "sysmon.web01.load1 0.25 1700000000\n"
                         "sysmon.web01.mem_used_bytes 12345678901 1700000000\n") == 0);
    sb_free(&o);
    return test_report("graphite");
}
