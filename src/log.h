#ifndef SYSMON_LOG_H
#define SYSMON_LOG_H

#include <stdarg.h>

enum {
    SM_LOG_ERR,
    SM_LOG_WARN,
    SM_LOG_INFO,
    SM_LOG_DEBUG,
};

void log_close(void);
void log_set_level(int level);
int log_level_from_str(const char *s);

void log_vmsg(int level, const char *fmt, va_list ap);
void log_msg(int level, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

#define log_err(...)   log_msg(SM_LOG_ERR, __VA_ARGS__)
#define log_warn(...)  log_msg(SM_LOG_WARN, __VA_ARGS__)
#define log_info(...)  log_msg(SM_LOG_INFO, __VA_ARGS__)
#define log_debug(...) log_msg(SM_LOG_DEBUG, __VA_ARGS__)

#endif
