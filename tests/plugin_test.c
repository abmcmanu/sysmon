/* plugin used by test_plugins */
#include <stdlib.h>

#include "sysmon/plugin.h"

static const sysmon_host_t *H;
static double value = 1;

static int t_init(const sysmon_host_t *host)
{
    H = host;
    const char *v = H->abi >= 2 ? H->option("plugin_test", "value") : NULL;
    if (v)
        value = atof(v);
    return 0;
}

static int t_collect(void)
{
    H->set("plugin_test_value", value);
    return 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "plugin_test", "0", t_init, t_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
