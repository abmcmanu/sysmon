/*
 * The smallest useful plugin, the one from docs/PLUGINS.md.
 *
 *   make examples
 *   [plugins] dir = build/examples, load = hello
 *   [plugin.hello] answer = 42
 */
#include <stdlib.h>

#include "sysmon/plugin.h"

static const sysmon_host_t *H;
static double answer = 42;

static int hello_init(const sysmon_host_t *host)
{
    H = host;
    const char *v = H->abi >= 2 ? H->option("hello", "answer") : NULL;
    if (v)
        answer = atof(v);
    H->set_help("hello_answer", "The answer.");
    return 0;
}

static int hello_collect(void)
{
    H->set("hello_answer", answer);
    return 0;
}

static const sysmon_plugin_t plugin = {
    SYSMON_PLUGIN_ABI, "hello", "1.0", hello_init, hello_collect, NULL,
};

SYSMON_PLUGIN_EXPORT(plugin)
