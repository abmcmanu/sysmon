#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "log.h"

void config_defaults(config_t *cfg)
{
    memset(cfg, 0, sizeof *cfg);
    cfg->interval = 5;
    cfg->log_level = SM_LOG_INFO;
    snprintf(cfg->mounts[0], CFG_PATH_MAX, "/");
    cfg->nmounts = 1;
    snprintf(cfg->socket_path, sizeof cfg->socket_path, "/tmp/sysmond.sock");
    snprintf(cfg->plugin_dir, CFG_PATH_MAX, "/usr/local/lib/sysmon/plugins");
    snprintf(cfg->http_bind, sizeof cfg->http_bind, "127.0.0.1");
}

static char *trim(char *s)
{
    while (isspace((unsigned char)*s))
        s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1]))
        *--e = '\0';
    return s;
}

static int copy(char *dst, size_t size, const char *src)
{
    if (strlen(src) >= size)
        return -1;
    memcpy(dst, src, strlen(src) + 1);
    return 0;
}

static int to_int(const char *s, int min, int max, int *out)
{
    char *end;
    errno = 0;
    long v = strtol(s, &end, 0);
    if (errno || *end || end == s || v < min || v > max)
        return -1;
    *out = (int)v;
    return 0;
}

static int seen_mount;

static int set_key(config_t *cfg, const char *section, const char *key, const char *val)
{
    if (strcmp(section, "core") == 0) {
        if (strcmp(key, "interval") == 0)
            return to_int(val, 1, 3600, &cfg->interval);
        if (strcmp(key, "log_level") == 0) {
            int l = log_level_from_str(val);
            if (l < 0)
                return -1;
            cfg->log_level = l;
            return 0;
        }
        if (strcmp(key, "log_file") == 0)
            return copy(cfg->log_file, sizeof cfg->log_file, val);
        if (strcmp(key, "pid_file") == 0)
            return copy(cfg->pid_file, sizeof cfg->pid_file, val);
        if (strcmp(key, "socket") == 0)
            return copy(cfg->socket_path, sizeof cfg->socket_path, val);
    }
    if (strcmp(section, "disk") == 0 && strcmp(key, "mount") == 0) {
        /* the first `mount` replaces the default "/" */
        if (!seen_mount) {
            cfg->nmounts = 0;
            seen_mount = 1;
        }
        if (cfg->nmounts == CFG_LIST_MAX)
            return -1;
        return copy(cfg->mounts[cfg->nmounts++], CFG_PATH_MAX, val);
    }
    if (strcmp(section, "plugins") == 0) {
        if (strcmp(key, "dir") == 0)
            return copy(cfg->plugin_dir, sizeof cfg->plugin_dir, val);
        if (strcmp(key, "load") == 0) {
            if (cfg->nplugins == CFG_LIST_MAX)
                return -1;
            return copy(cfg->plugins[cfg->nplugins++], sizeof cfg->plugins[0], val);
        }
    }
    if (strcmp(section, "http") == 0) {
        if (strcmp(key, "port") == 0)
            return to_int(val, 0, 65535, &cfg->http_port);
        if (strcmp(key, "bind") == 0)
            return copy(cfg->http_bind, sizeof cfg->http_bind, val);
    }
    if (strcmp(section, "alerts") == 0) {
        if (strcmp(key, "rule") == 0) {
            if (cfg->nalerts == CFG_ALERTS_MAX)
                return -1;
            return copy(cfg->alerts[cfg->nalerts++], CFG_RULE_MAX, val);
        }
        if (strcmp(key, "exec") == 0)
            return copy(cfg->alert_exec, sizeof cfg->alert_exec, val);
    }
    return 1; /* unknown key */
}

int config_load(config_t *cfg, const char *path)
{
    char line[512], section[32] = "core";
    int lineno = 0, errors = 0;

    seen_mount = 0;
    FILE *f = fopen(path, "r");
    if (!f) {
        log_err("config: %s: %s", path, strerror(errno));
        return -1;
    }

    while (fgets(line, sizeof line, f)) {
        lineno++;
        char *s = trim(line);
        if (*s == '\0' || *s == '#' || *s == ';')
            continue;

        if (*s == '[') {
            char *end = strchr(s, ']');
            if (!end) {
                log_err("%s:%d: missing ']'", path, lineno);
                errors++;
                continue;
            }
            *end = '\0';
            snprintf(section, sizeof section, "%s", trim(s + 1));
            continue;
        }

        char *eq = strchr(s, '=');
        if (!eq) {
            log_err("%s:%d: expected 'key = value'", path, lineno);
            errors++;
            continue;
        }
        *eq = '\0';
        char *key = trim(s);
        char *val = trim(eq + 1);

        int rc = set_key(cfg, section, key, val);
        if (rc > 0) {
            log_err("%s:%d: unknown key '%s' in [%s]", path, lineno, key, section);
            errors++;
        } else if (rc < 0) {
            log_err("%s:%d: invalid value for '%s': %s", path, lineno, key, val);
            errors++;
        }
    }
    fclose(f);
    return errors ? -1 : 0;
}
