#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "log.h"
#include "test.h"

static config_t cfg;

static const char *write_tmp(const char *text)
{
    static char path[] = "/tmp/sysmon-test-XXXXXX";
    strcpy(path, "/tmp/sysmon-test-XXXXXX");
    int fd = mkstemp(path);
    if (fd < 0)
        return NULL;
    if (write(fd, text, strlen(text)) < 0)
        return NULL;
    close(fd);
    return path;
}

int main(void)
{
    const char *p;

    log_set_level(-1); /* errors are expected below, keep output clean */

    config_defaults(&cfg);
    CHECK(cfg.interval == 5);
    CHECK(cfg.nmounts == 1 && strcmp(cfg.mounts[0], "/") == 0);

    p = write_tmp("# comment\n"
                  "[core]\n"
                  "interval = 2\n"
                  "  log_level = debug   \n"
                  "socket = /run/sysmond.sock\n"
                  "\n"
                  "[disk]\n"
                  "mount = /home\n"
                  "mount = /var\n"
                  "; other comment\n"
                  "[plugins]\n"
                  "load = net\n");
    CHECK(p != NULL);
    config_defaults(&cfg);
    CHECK(config_load(&cfg, p) == 0);
    CHECK(cfg.interval == 2);
    CHECK(cfg.log_level == SM_LOG_DEBUG);
    CHECK(strcmp(cfg.socket_path, "/run/sysmond.sock") == 0);
    CHECK(cfg.nmounts == 2);
    CHECK(strcmp(cfg.mounts[1], "/var") == 0);
    CHECK(cfg.nplugins == 1 && strcmp(cfg.plugins[0], "net") == 0);
    unlink(p);

    /* loading the same file twice does not duplicate list entries */
    p = write_tmp("[disk]\nmount = /home\n");
    config_defaults(&cfg);
    CHECK(config_load(&cfg, p) == 0);
    config_defaults(&cfg);
    CHECK(config_load(&cfg, p) == 0);
    CHECK(cfg.nmounts == 1);
    unlink(p);

    p = write_tmp("[core]\ninterval = 0\n");
    config_defaults(&cfg);
    CHECK(config_load(&cfg, p) < 0);
    unlink(p);

    p = write_tmp("[core]\nno equals sign\n");
    config_defaults(&cfg);
    CHECK(config_load(&cfg, p) < 0);
    unlink(p);

    p = write_tmp("[core]\nintreval = 3\n");
    config_defaults(&cfg);
    CHECK(config_load(&cfg, p) < 0); /* typos are errors, not silence */
    unlink(p);

    CHECK(config_load(&cfg, "/nonexistent/sysmond.conf") < 0);

    p = write_tmp("[core]\ninterval = 2m\n");
    config_defaults(&cfg);
    CHECK(config_load(&cfg, p) == 0 && cfg.interval == 120);
    unlink(p);
    p = write_tmp("[core]\ninterval = 10x\n");
    CHECK(config_load(&cfg, p) < 0);
    unlink(p);
    p = write_tmp("[core]\ninterval = 2h\n");
    CHECK(config_load(&cfg, p) < 0); /* more than an hour */
    unlink(p);

    char inc[64];
    p = write_tmp("[core]\ninterval = 7\n");
    snprintf(inc, sizeof inc, "%s", p);
    char main_conf[128];
    snprintf(main_conf, sizeof main_conf, "[disk]\nmount = /a\ninclude = %s\n", inc);
    p = write_tmp(main_conf);
    config_defaults(&cfg);
    CHECK(config_load(&cfg, p) == 0);
    CHECK(cfg.interval == 7 && cfg.nmounts == 1);
    unlink(p);
    unlink(inc);

    p = write_tmp("[core]\ninclude = /nonexistent/sysmond.d/*.conf\n");
    CHECK(config_load(&cfg, p) == 0); /* an empty conf.d is fine */
    unlink(p);

    return test_report("config");
}
