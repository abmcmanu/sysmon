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

    return test_report("metrics");
}
