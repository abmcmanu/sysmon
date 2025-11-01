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

    sb_free(&o);
    return test_report("json");
}
