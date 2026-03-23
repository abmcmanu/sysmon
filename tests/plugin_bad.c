/* built for an ABI the daemon does not know: must be refused */
#include <stddef.h>

#include "sysmon/plugin.h"

static int b_collect(void)
{
    return 0;
}

static const sysmon_plugin_t plugin = {
    99, "plugin_bad", "0", NULL, b_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
