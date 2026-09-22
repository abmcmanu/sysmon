#include <math.h>
#include <stdio.h>
#include <string.h>

#include "graphite.h"
#include "metrics.h"

void graphite_format(sb_t *out, const char *pfx, time_t ts)
{
    for (size_t i = 0; i < metrics_count(); i++) {
        const metric_t *m = metrics_at(i);
        if (!isnan(m->value))
            sb_printf(out, "%s.%s %.15g %lld\n", pfx, m->name, m->value, (long long)ts);
    }
}
