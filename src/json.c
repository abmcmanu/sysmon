#include <math.h>
#include <stdio.h>
#include <time.h>

#include "json.h"
#include "metrics.h"
#include "version.h"

void json_string(sb_t *out, const char *s)
{
    sb_puts(out, "\"");
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        switch (c) {
        case '"':  sb_puts(out, "\\\""); break;
        case '\\': sb_puts(out, "\\\\"); break;
        case '\n': sb_puts(out, "\\n"); break;
        case '\r': sb_puts(out, "\\r"); break;
        case '\t': sb_puts(out, "\\t"); break;
        default:
            if (c < 0x20)
                sb_printf(out, "\\u%04x", c);
            else
                sb_append(out, (const char *)&c, 1);
        }
    }
    sb_puts(out, "\"");
}

void json_number(sb_t *out, double v)
{
    if (isnan(v) || isinf(v))
        sb_puts(out, "null"); /* JSON has no NaN */
    else
        sb_printf(out, "%.17g", v);
}

void json_metrics(sb_t *out)
{
    sb_puts(out, "{\"version\":");
    json_string(out, SYSMON_VERSION);
    sb_printf(out, ",\"timestamp\":%lld,\"metrics\":{", (long long)time(NULL));
    for (size_t i = 0; i < metrics_count(); i++) {
        const metric_t *m = metrics_at(i);
        if (i)
            sb_puts(out, ",");
        json_string(out, m->name);
        sb_puts(out, ":");
        json_number(out, m->value);
    }
    sb_puts(out, "}}\n");
}
