/* libFuzzer target: the configuration parser */
#include <stdint.h>
#include <stdio.h>

#include "config.h"
#include "log.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static const char path[] = "/tmp/sysmon-fuzz.conf";
    static config_t cfg;

    log_set_level(-1);
    FILE *f = fopen(path, "w");
    if (!f)
        return 0;
    fwrite(data, 1, size, f);
    fclose(f);
    config_defaults(&cfg);
    config_load(&cfg, path);
    return 0;
}
