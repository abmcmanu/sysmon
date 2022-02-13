#include <string.h>

#include "alert.h"
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

    return test_report("alert");
}
