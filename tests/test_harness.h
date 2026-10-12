#ifndef TETONOR_TESTS_TEST_HARNESS_H
#define TETONOR_TESTS_TEST_HARNESS_H

#include <stdio.h>
#include <stdlib.h>

static int test_harness_failures;

#define TEST_HARNESS_FAIL(...)                                                 \
    do                                                                         \
    {                                                                          \
        fprintf(stderr, "%s:%d: ", __FILE__, __LINE__);                        \
        fprintf(stderr, __VA_ARGS__);                                          \
        fprintf(stderr, "\n");                                                 \
        test_harness_failures++;                                               \
        return;                                                                \
    } while (0)

#define ASSERT_TRUE(condition)                                                 \
    do                                                                         \
    {                                                                          \
        if (!(condition))                                                      \
        {                                                                      \
            TEST_HARNESS_FAIL("expected true: %s", #condition);                \
        }                                                                      \
    } while (0)

#define ASSERT_FALSE(condition)                                                \
    do                                                                         \
    {                                                                          \
        if (condition)                                                         \
        {                                                                      \
            TEST_HARNESS_FAIL("expected false: %s", #condition);               \
        }                                                                      \
    } while (0)

#define ASSERT_EQ_INT(actual, expected)                                        \
    do                                                                         \
    {                                                                          \
        long long harness_actual   = (long long)(actual);                      \
        long long harness_expected = (long long)(expected);                    \
        if (harness_actual != harness_expected)                                \
        {                                                                      \
            TEST_HARNESS_FAIL("%s == %s: actual %lld, expected %lld", #actual, \
                              #expected, harness_actual, harness_expected);    \
        }                                                                      \
    } while (0)

#define ASSERT_NOT_NULL(pointer)                                               \
    do                                                                         \
    {                                                                          \
        if ((pointer) == NULL)                                                 \
        {                                                                      \
            TEST_HARNESS_FAIL("expected non-NULL: %s", #pointer);              \
        }                                                                      \
    } while (0)

#define ASSERT_NULL(pointer)                                                   \
    do                                                                         \
    {                                                                          \
        if ((pointer) != NULL)                                                 \
        {                                                                      \
            TEST_HARNESS_FAIL("expected NULL: %s", #pointer);                  \
        }                                                                      \
    } while (0)

#define RUN_TEST(test_function)                                                \
    do                                                                         \
    {                                                                          \
        int harness_failures_before = test_harness_failures;                   \
        test_function();                                                       \
        printf("%s %s\n",                                                      \
               test_harness_failures == harness_failures_before ? "PASS"       \
                                                                : "FAIL",      \
               #test_function);                                                \
    } while (0)

static inline int test_harness_result(void)
{
    return test_harness_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

#endif
