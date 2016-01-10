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
}
