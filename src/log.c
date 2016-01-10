#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "log.h"

static FILE *out;
static int threshold = SM_LOG_INFO;
static const char *const names[] = { "error", "warn", "info", "debug" };

void log_close(void)
{
    if (out && out != stderr)
        fclose(out);
    out = NULL;
}

void log_set_level(int level)
{
    threshold = level;
}

int log_level_from_str(const char *s)
{
    for (int i = 0; i < (int)(sizeof names / sizeof names[0]); i++)
        if (strcmp(s, names[i]) == 0)
            return i;
    return -1;
}

void log_vmsg(int level, const char *fmt, va_list ap)
{
    if (level < SM_LOG_ERR)
        level = SM_LOG_ERR;
    if (level > SM_LOG_DEBUG)
        level = SM_LOG_DEBUG;
    if (level > threshold)
        return;

    FILE *o = out ? out : stderr;
    char ts[32];
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    strftime(ts, sizeof ts, "%Y-%m-%dT%H:%M:%S", &tm);

    fprintf(o, "%s [%s] ", ts, names[level]);
    vfprintf(o, fmt, ap);
    fputc('\n', o);
}

void log_msg(int level, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_vmsg(level, fmt, ap);
    va_end(ap);
}
