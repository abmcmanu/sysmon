#include <string.h>

#include "sb.h"
#include "test.h"

int main(void)
{
    sb_t sb;

    sb_init(&sb);
    CHECK(sb.len == 0);
    sb_puts(&sb, "hello");
    sb_printf(&sb, " %s %d", "world", 42);
    CHECK(strcmp(sb.data, "hello world 42") == 0);
    CHECK(sb.len == strlen("hello world 42"));

    /* force a few reallocations, and the vsnprintf slow path */
    char big[1000];
    memset(big, 'x', sizeof big - 1);
    big[sizeof big - 1] = '\0';
    sb_printf(&sb, "%s", big);
    CHECK(sb.len == 14 + 999);
    CHECK(sb.data[sb.len] == '\0');
    CHECK(sb.data[sb.len - 1] == 'x');

    sb_reset(&sb);
    CHECK(sb.len == 0 && sb.data[0] == '\0');
    sb_free(&sb);
    CHECK(sb.data == NULL);

    return test_report("sb");
}
