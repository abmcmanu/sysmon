#include <string.h>

#include "config.h"
#include "log.h"
#include "metrics.h"
#include "plugins.h"
#include "test.h"

static config_t cfg;

int main(int argc, char **argv)
{
    log_set_level(-1);
    config_defaults(&cfg);
    snprintf(cfg.plugin_dir, sizeof cfg.plugin_dir, "%s", argc > 1 ? argv[1] : "build/tests");
    snprintf(cfg.plugins[cfg.nplugins++], sizeof cfg.plugins[0], "plugin_test");
    snprintf(cfg.plugins[cfg.nplugins++], sizeof cfg.plugins[0], "plugin_bad");
    snprintf(cfg.plugins[cfg.nplugins++], sizeof cfg.plugins[0], "does_not_exist");
    snprintf(cfg.opts[0].plugin, sizeof cfg.opts[0].plugin, "plugin_test");
    snprintf(cfg.opts[0].key, sizeof cfg.opts[0].key, "value");
    snprintf(cfg.opts[0].value, sizeof cfg.opts[0].value, "42");
    cfg.nopts = 1;

    CHECK(plugins_load(&cfg) < 0); /* does_not_exist */
    CHECK(plugins_count() == 1);
    plugins_collect();
    const metric_t *m = metrics_get("plugin_test_value");
    CHECK(m && m->value == 42);
    plugins_unload();
    CHECK(plugins_count() == 0);

    return test_report("plugins");
}
