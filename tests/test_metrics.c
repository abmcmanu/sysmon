#include <stdio.h>
#include <string.h>

#include "log.h"
#include "metrics.h"
#include "test.h"

int main(void)
{
    log_set_level(-1);

    CHECK(metrics_count() == 0);
    CHECK(metrics_get("a") == NULL);
    metrics_set("a", 1);
    metrics_set("a", 2);
    CHECK(metrics_count() == 1);
    CHECK(metrics_get("a")->value == 2);
    CHECK(ring_len(&metrics_get("a")->hist) == 2);

    CHECK(metric_name_valid("cpu_usage_percent"));
    CHECK(metric_name_valid("_x9"));
    CHECK(!metric_name_valid("9x"));
    CHECK(!metric_name_valid("a-b"));
    CHECK(!metric_name_valid(""));
    CHECK(metrics_set("bad name", 1) == NULL);

    metrics_set_help("b", "help text");
    CHECK(isnan(metrics_get("b")->value));
    CHECK(strcmp(metrics_get("b")->help, "help text") == 0);

    char name[32];
    for (int i = 0; i < METRICS_MAX + 10; i++) {
        snprintf(name, sizeof name, "m%d", i);
        metrics_set(name, i);
    }
    CHECK(metrics_count() == METRICS_MAX);

    return test_report("metrics");
}
