#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sb.h"

void sb_init(sb_t *sb)
{
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

void sb_free(sb_t *sb)
{
    free(sb->data);
    sb_init(sb);
}

void sb_reset(sb_t *sb)
{
    sb->len = 0;
    if (sb->data)
        sb->data[0] = '\0';
}

static int grow(sb_t *sb, size_t extra)
{
    size_t need = sb->len + extra + 1;
    if (need <= sb->cap)
        return 0;
    size_t cap = sb->cap ? sb->cap : 256;
    while (cap < need)
        cap *= 2;
    char *p = realloc(sb->data, cap);
    if (!p)
        return -1;
    sb->data = p;
    sb->cap = cap;
    return 0;
}

int sb_append(sb_t *sb, const char *s, size_t n)
{
    if (grow(sb, n) < 0)
        return -1;
    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
    return 0;
}

int sb_puts(sb_t *sb, const char *s)
{
    return sb_append(sb, s, strlen(s));
}

int sb_printf(sb_t *sb, const char *fmt, ...)
{
    va_list ap;
    char small[256];

    va_start(ap, fmt);
    int n = vsnprintf(small, sizeof small, fmt, ap);
    va_end(ap);
    if (n < 0)
        return -1;
    if ((size_t)n < sizeof small)
        return sb_append(sb, small, (size_t)n);

    if (grow(sb, (size_t)n) < 0)
        return -1;
    va_start(ap, fmt);
    vsnprintf(sb->data + sb->len, (size_t)n + 1, fmt, ap);
    va_end(ap);
    sb->len += (size_t)n;
    return 0;
}
