/* libFuzzer target: one control protocol command */
#include <stdint.h>
#include <string.h>

#include "log.h"
#include "metrics.h"
#include "sb.h"
#include "server.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static int ready;
    char line[1024];
    sb_t out;

    if (!ready) {
        log_set_level(-1);
        for (int i = 0; i < 20; i++)
            metrics_set("load1", i);
        metrics_set("cpu_usage_percent", 12.5);
        ready = 1;
    }
    if (size >= sizeof line)
        size = sizeof line - 1;
    memcpy(line, data, size);
    line[size] = '\0';

    sb_init(&out);
    server_exec(line, &out);
    sb_free(&out);
    return 0;
}
