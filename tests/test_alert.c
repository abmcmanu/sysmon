#include <string.h>

#include "alert.h"
#include "metrics.h"
#include "test.h"

int main(void)
{
    alert_t a;

    CHECK(alert_parse("cpu_usage_percent > 90", &a) == 0);
    CHECK(strcmp(a.metric, "cpu_usage_percent") == 0);
    CHECK(a.op == ALERT_GT && a.threshold == 90 && a.for_ticks == 1);

    CHECK(alert_parse("disk_root_used_percent >= 95.5 for 3", &a) == 0);
    CHECK(a.op == ALERT_GE && a.threshold == 95.5 && a.for_ticks == 3);

    CHECK(alert_parse("load1 <= 0.1", &a) == 0 && a.op == ALERT_LE);
    CHECK(alert_parse("load1 == 1", &a) < 0);
    CHECK(alert_parse("load1 >", &a) < 0);
    CHECK(alert_parse("load1 > 1 for", &a) < 0);
    CHECK(alert_parse("load1 > 1 for 0", &a) < 0);
    CHECK(alert_parse("load1 > 1 for 2 minutes", &a) < 0);
    CHECK(alert_parse("load1 > 1 for 30s", &a) == 0 && a.for_seconds == 30);
    CHECK(alert_parse("load1 > 1 for 5m", &a) == 0 && a.for_seconds == 300);
    CHECK(alert_parse("load1 > 1 for 3 ", &a) == 0 && a.for_seconds == 0 && a.for_ticks == 3);
    CHECK(alert_parse("load1 > 1 for 5x", &a) < 0);
    CHECK(alert_ticks(30, 5) == 6);
    CHECK(alert_ticks(31, 5) == 7);
    CHECK(alert_ticks(1, 60) == 1);

    /* state machine: fires after 3 consecutive hits, resolves on the first miss */
    CHECK(alert_parse("x > 10 for 3", &a) == 0);
    CHECK(alert_update(&a, 11) == 0);
    CHECK(alert_update(&a, 12) == 0);
    CHECK(alert_update(&a, 5) == 0);   /* streak broken */
    CHECK(alert_update(&a, 11) == 0);
    CHECK(alert_update(&a, 11) == 0);
    CHECK(alert_update(&a, 11) == 1);  /* fires */
    CHECK(a.firing);
    CHECK(alert_update(&a, 50) == 0);  /* still firing, no new event */
    CHECK(alert_update(&a, NAN) == 0); /* missing sample changes nothing */
    CHECK(alert_update(&a, 10) == -1); /* resolves */
    CHECK(!a.firing);


    char rules[1][256] = { "y > 10" };
    metrics_set("y", 11);
    CHECK(alerts_load(rules, 1, "") == 0);
    alerts_eval();
    CHECK(alerts_firing() == 1);
    CHECK(alerts_load(rules, 1, "") == 0); /* reload with the same rule */
    CHECK(alerts_firing() == 1);
    snprintf(rules[0], sizeof rules[0], "y > 100");
    CHECK(alerts_load(rules, 1, "") == 0); /* a changed rule starts over */
    CHECK(alerts_firing() == 0);
    return test_report("alert");
}
