/* check.h - assertions shared by the test suites.

   CHECK records a failure and carries on, which is what you want for
   independent facts: one broken sort order should not hide the other five.

   REQUIRE stops the run. Some assertions are preconditions for the lines
   after them - if `dialog.line` is NULL then every following dereference is a
   segfault, and a suite that crashes reports nothing at all. That is exactly
   how 1.4's first test run failed: a stale ordering left the counter open, the
   menu never opened, and the next line read through a null pointer. The
   information was there and the crash threw it away. */
#ifndef TEST_CHECK_H
#define TEST_CHECK_H

#include <stdio.h>
#include <stdlib.h>

static int g_fails = 0;

#define CHECK(c, m) \
    do { if (!(c)) { printf("  FAIL %s\n", (m)); g_fails++; } } while (0)

#define REQUIRE(c, m) \
    do { \
        if (!(c)) { \
            printf("  FAIL %s\n", (m)); \
            printf("  (precondition - stopping here)\n"); \
            g_fails++; \
            printf("\nFAILED (%d failures)\n", g_fails); \
            exit(1); \
        } \
    } while (0)

#define REPORT() \
    do { \
        printf("\n%s (%d failures)\n", \
               g_fails ? "FAILED" : "ALL CHECKS PASSED", g_fails); \
        return g_fails != 0; \
    } while (0)

#endif /* TEST_CHECK_H */
