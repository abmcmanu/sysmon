#ifndef SYSMON_SB_H
#define SYSMON_SB_H

#include <stddef.h>

/* Growable string buffer, always NUL-terminated once non-empty. */
typedef struct sb {
    char *data;
    size_t len;
    size_t cap;
} sb_t;

void sb_init(sb_t *sb);
void sb_free(sb_t *sb);
void sb_reset(sb_t *sb);
int sb_append(sb_t *sb, const char *s, size_t n);
int sb_puts(sb_t *sb, const char *s);
int sb_printf(sb_t *sb, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

#endif
