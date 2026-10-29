#ifndef TEST_UTIL_H
#define TEST_UTIL_H

/*
 * Minimal dependency-free test helpers, so the tests build with plain gcc/clang
 * on every OS in CI. Each test file is its own program: it exits non-zero if
 * any CHECK failed.
 */
#include <stdio.h>

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(condition)                                                    \
    do {                                                                    \
        g_checks++;                                                         \
        if (!(condition)) {                                                 \
            g_failures++;                                                   \
            fprintf(stderr, "  FAIL %s:%d: %s\n",                           \
                    __FILE__, __LINE__, #condition);                        \
        }                                                                   \
    } while (0)

#define TEST_RUN(test_function)                                             \
    do {                                                                    \
        int failures_before_ = g_failures;                                  \
        test_function();                                                    \
        printf("  [%s] %s\n",                                               \
               g_failures == failures_before_ ? "PASS" : "FAIL",            \
               #test_function);                                             \
    } while (0)

#define TEST_SUMMARY()                                                      \
    (printf("%d checks, %d failed\n", g_checks, g_failures),                \
     g_failures != 0)

#endif
