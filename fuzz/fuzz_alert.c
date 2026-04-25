/* libFuzzer target: alert rule parser and state machine */
#include <stdint.h>
#include <string.h>

#include "alert.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char rule[256];
    alert_t a;

    if (size >= sizeof rule)
        size = sizeof rule - 1;
    memcpy(rule, data, size);
    rule[size] = '\0';
    if (alert_parse(rule, &a) == 0)
        for (int i = 0; i < 10; i++)
            alert_update(&a, i % 3 ? a.threshold + 1 : a.threshold - 1);
    return 0;
}
