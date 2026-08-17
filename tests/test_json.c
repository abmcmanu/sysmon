#include <string.h>

#include "json.h"
#include "metrics.h"
#include "sb.h"
#include "test.h"

int main(void)
{
    sb_t o;

    sb_init(&o);
    json_string(&o, "a\"b\\c\n\x01");
    CHECK(strcmp(o.data, "\"a\\\"b\\\\c\\n\\u0001\"") == 0);

    sb_reset(&o);
    json_number(&o, NAN);
    CHECK(strcmp(o.data, "null") == 0);
    sb_reset(&o);
    json_number(&o, 0.25);
    CHECK(strcmp(o.data, "0.25") == 0);

    sb_reset(&o);
    metrics_set("x", 1);
    json_metrics(&o);
    CHECK(strncmp(o.data, "{\"version\":", 11) == 0);
    CHECK(strstr(o.data, "\"metrics\":{\"x\":1}") != NULL);
    CHECK(strstr(o.data, ",\"host\":\"") != NULL);

    metrics_set("x", 2);
    sb_reset(&o);
    json_history(&o, metrics_get("x"), 10);
    CHECK(strcmp(o.data, "{\"metric\":\"x\",\"values\":[1,2]}\n") == 0);
    sb_reset(&o);
    json_history(&o, metrics_get("x"), 1);
    CHECK(strcmp(o.data, "{\"metric\":\"x\",\"values\":[2]}\n") == 0);

    sb_free(&o);
    return test_report("json");
}
