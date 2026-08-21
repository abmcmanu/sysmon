#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "test.h"

int main(void)
{
    CHECK(log_level_from_str("error") == SM_LOG_ERR);
    CHECK(log_level_from_str("debug") == SM_LOG_DEBUG);
    CHECK(log_level_from_str("verbose") < 0);

    return test_report("log");
}
