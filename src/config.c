#include <ctype.h>
#include <errno.h>
#include <glob.h>
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
    cfg->socket_mode = 0660;
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

/* "30", "30s", "5m", "1h" -> seconds */
static int to_seconds(const char *s, int min, int max, int *out)
{
    char *end;

    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno || end == s || v < 0)
        return -1;
    if (*end == 's') {
        end++;
    } else if (*end == 'm') {
        v *= 60;
        end++;
    } else if (*end == 'h') {
        v *= 3600;
        end++;
    }
    if (*end || v < min || v > max)
        return -1;
    *out = (int)v;
    return 0;
}

static int set_key(config_t *cfg, const char *section, const char *key, const char *val)
{
    if (strcmp(section, "core") == 0) {
        if (strcmp(key, "interval") == 0)
            return to_seconds(val, 1, 3600, &cfg->interval);
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
        if (strcmp(key, "socket_mode") == 0)
            return to_int(val, 0, 0777, &cfg->socket_mode);
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

static int load_file(config_t *cfg, const char *path, int depth);

static int include(config_t *cfg, const char *from, const char *pattern, int depth)
{
    glob_t g;
    int errors = 0;
    char buf[CFG_PATH_MAX * 2];

    /* relative patterns are relative to the including file */
    const char *slash = strrchr(from, '/');
    if (pattern[0] != '/' && slash) {
        if (snprintf(buf, sizeof buf, "%.*s/%s", (int)(slash - from), from, pattern) >=
            (int)sizeof buf) {
            log_err("config: include %s: path too long", pattern);
            return -1;
        }
        pattern = buf;
    }
    if (depth >= 4) {
        log_err("config: includes nested too deep at %s", pattern);
        return -1;
    }
    int rc = glob(pattern, 0, NULL, &g);
    if (rc == GLOB_NOMATCH)
        return 0;
    if (rc != 0) {
        log_err("config: include %s: glob failed", pattern);
        return -1;
    }
    for (size_t i = 0; i < g.gl_pathc; i++)
        if (load_file(cfg, g.gl_pathv[i], depth + 1) < 0)
            errors++;
    globfree(&g);
    return errors ? -1 : 0;
}

static int load_file(config_t *cfg, const char *path, int depth)
{
    char line[512], section[32] = "core";
    int lineno = 0, errors = 0;

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
        if (strcmp(key, "include") == 0) {
            if (include(cfg, path, val, depth) < 0)
                errors++;
            continue;
        }

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

int config_load(config_t *cfg, const char *path)
{
    seen_mount = 0;
    return load_file(cfg, path, 0);
}
