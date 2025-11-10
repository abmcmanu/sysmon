#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/statvfs.h>

#include "collector.h"
#include "log.h"
#include "metrics.h"

void disk_metric_name(const char *mnt, char *out, size_t size)
{
    size_t j = 0;

    while (*mnt == '/')
        mnt++;
    if (!*mnt) {
        snprintf(out, size, "root");
        return;
    }
    for (; *mnt && j + 1 < size; mnt++)
        out[j++] = isalnum((unsigned char)*mnt) ? (char)tolower((unsigned char)*mnt) : '_';
    out[j] = '\0';
}

int collect_disks(const config_t *cfg)
{
    char name[48], metric[96];
    int errors = 0;

    for (int i = 0; i < cfg->nmounts; i++) {
        struct statvfs st;
        if (statvfs(cfg->mounts[i], &st) < 0) {
            log_warn("disk: statvfs(%s): %s", cfg->mounts[i], strerror(errno));
            errors++;
            continue;
        }
        double frsize = (double)st.f_frsize;
        double used = ((double)st.f_blocks - (double)st.f_bfree) * frsize;
        double avail = (double)st.f_bavail * frsize;
        /* same formula as df(1): reserved blocks do not count as free */
        double pct = used + avail > 0 ? 100.0 * used / (used + avail) : 0.0;

        disk_metric_name(cfg->mounts[i], name, sizeof name);
        snprintf(metric, sizeof metric, "disk_%s_used_percent", name);
        metrics_set(metric, pct);
        snprintf(metric, sizeof metric, "disk_%s_free_bytes", name);
        metrics_set(metric, avail);
    }
    return errors ? -1 : 0;
}
