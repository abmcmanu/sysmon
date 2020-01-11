/*
 * Minimal test harness: one binary per test file, no dependencies.
 */
#ifndef SYSMON_TEST_H
#define SYSMON_TEST_H

#include <math.h>
#include <stdio.h>

static int tests_run, tests_failed;

#define CHECK(cond)                                                          \
    do {                                                                     \
        tests_run++;                                                         \
        if (!(cond)) {                                                       \
            tests_failed++;                                                  \
            fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, \
                    #cond);                                                  \
        }                                                                    \
    } while (0)

#define CHECK_NEAR(a, b, eps) CHECK(fabs((double)(a) - (double)(b)) < (eps))

static inline int test_report(const char *name)
{
    printf("%-12s %3d checks, %d failed\n", name, tests_run, tests_failed);
    return tests_failed ? 1 : 0;
}

#endif
