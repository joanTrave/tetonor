#include <stdbool.h>
#include <time.h>

#include "../test_harness.h"
#include "../../src/ui/read_timeout.h"

#define TEST_FIRST_ERROR_TIME ((time_t)100)
#define TEST_MILLISECONDS_PER_SECOND 1000
#define TEST_ERRORS_PER_SECOND 4
#define TEST_SLOW_ERROR_COUNT 400

/* Feeds count errors, all at time now, and reports whether any was flagged. */
static bool errors_flag_closed_input(ReadTimeoutWatch* watch, int count,
                                     time_t now)
{
    bool flagged = false;
    for (int i = 0; i < count; i++)
    {
        if (note_read_error(watch, now))
        {
            flagged = true;
        }
    }
    return flagged;
}

static void test_read_timeout_constants_cannot_flag_real_timeouts(void)
{
    ASSERT_EQ_INT(TETONOR_READ_TIMEOUT_MS, 250);
    ASSERT_EQ_INT(TETONOR_READ_CLOSED_STREAK, 40);
    ASSERT_EQ_INT(TETONOR_READ_CLOSED_WINDOW_SECONDS, 2);
    ASSERT_TRUE((TETONOR_READ_CLOSED_STREAK - 1) * TETONOR_READ_TIMEOUT_MS /
                    TEST_MILLISECONDS_PER_SECOND >
                TETONOR_READ_CLOSED_WINDOW_SECONDS + 1);
}

static void test_init_read_timeout_watch_starts_without_errors(void)
{
    ReadTimeoutWatch watch;
    watch.consecutive_errors = 5;
    init_read_timeout_watch(&watch);
    ASSERT_EQ_INT(watch.consecutive_errors, 0);
}

static void test_note_read_error_instant_errors_mean_closed(void)
{
    ReadTimeoutWatch watch;
    init_read_timeout_watch(&watch);
    for (int i = 0; i < TETONOR_READ_CLOSED_STREAK - 1; i++)
    {
        ASSERT_FALSE(note_read_error(&watch, TEST_FIRST_ERROR_TIME));
    }
    ASSERT_TRUE(note_read_error(&watch, TEST_FIRST_ERROR_TIME));
}

static void test_note_read_error_few_errors_never_close(void)
{
    ReadTimeoutWatch watch;
    init_read_timeout_watch(&watch);
    ASSERT_FALSE(errors_flag_closed_input(
        &watch, TETONOR_READ_CLOSED_STREAK - 1, TEST_FIRST_ERROR_TIME));
}

static void test_note_read_error_errors_within_window_mean_closed(void)
{
    ReadTimeoutWatch watch;
    init_read_timeout_watch(&watch);
    ASSERT_FALSE(errors_flag_closed_input(
        &watch, TETONOR_READ_CLOSED_STREAK - 1, TEST_FIRST_ERROR_TIME));
    ASSERT_TRUE(note_read_error(
        &watch, TEST_FIRST_ERROR_TIME + TETONOR_READ_CLOSED_WINDOW_SECONDS));
}

static void test_note_read_error_just_past_the_window_does_not_close(void)
{
    ReadTimeoutWatch watch;
    init_read_timeout_watch(&watch);
    errors_flag_closed_input(&watch, TETONOR_READ_CLOSED_STREAK - 1,
                             TEST_FIRST_ERROR_TIME);
    ASSERT_FALSE(
        note_read_error(&watch, TEST_FIRST_ERROR_TIME +
                                    TETONOR_READ_CLOSED_WINDOW_SECONDS + 1));
}

static void test_note_read_error_slow_timeouts_never_close(void)
{
    ReadTimeoutWatch watch;
    init_read_timeout_watch(&watch);
    for (int i = 0; i < TEST_SLOW_ERROR_COUNT; i++)
    {
        time_t now = TEST_FIRST_ERROR_TIME + i / TEST_ERRORS_PER_SECOND;
        ASSERT_FALSE(note_read_error(&watch, now));
    }
}

static void test_note_read_error_window_restarts_after_slow_streak(void)
{
    ReadTimeoutWatch watch;
    init_read_timeout_watch(&watch);
    ASSERT_FALSE(errors_flag_closed_input(
        &watch, TETONOR_READ_CLOSED_STREAK - 1, TEST_FIRST_ERROR_TIME));
    ASSERT_FALSE(note_read_error(&watch, TEST_FIRST_ERROR_TIME + 10));

    time_t later = TEST_FIRST_ERROR_TIME + 100;
    ASSERT_FALSE(errors_flag_closed_input(
        &watch, TETONOR_READ_CLOSED_STREAK - 1, later));
    ASSERT_TRUE(note_read_error(&watch, later));
}

static void test_note_key_read_resets_streak(void)
{
    ReadTimeoutWatch watch;
    init_read_timeout_watch(&watch);
    ASSERT_FALSE(errors_flag_closed_input(
        &watch, TETONOR_READ_CLOSED_STREAK - 1, TEST_FIRST_ERROR_TIME));
    note_key_read(&watch);
    ASSERT_EQ_INT(watch.consecutive_errors, 0);
    ASSERT_FALSE(errors_flag_closed_input(
        &watch, TETONOR_READ_CLOSED_STREAK - 1, TEST_FIRST_ERROR_TIME));
}

static void test_note_key_read_restarts_the_window(void)
{
    ReadTimeoutWatch watch;
    init_read_timeout_watch(&watch);
    errors_flag_closed_input(&watch, 10, TEST_FIRST_ERROR_TIME);
    note_key_read(&watch);
    time_t later = TEST_FIRST_ERROR_TIME + 50;
    ASSERT_FALSE(errors_flag_closed_input(
        &watch, TETONOR_READ_CLOSED_STREAK - 1, later));
    ASSERT_TRUE(note_read_error(&watch, later));
}

static void test_note_read_error_clock_backwards_does_not_close(void)
{
    ReadTimeoutWatch watch;
    init_read_timeout_watch(&watch);
    ASSERT_FALSE(errors_flag_closed_input(
        &watch, TETONOR_READ_CLOSED_STREAK - 1, TEST_FIRST_ERROR_TIME));
    ASSERT_FALSE(note_read_error(&watch, TEST_FIRST_ERROR_TIME - 10));
}

int main(void)
{
    RUN_TEST(test_read_timeout_constants_cannot_flag_real_timeouts);
    RUN_TEST(test_init_read_timeout_watch_starts_without_errors);
    RUN_TEST(test_note_read_error_instant_errors_mean_closed);
    RUN_TEST(test_note_read_error_few_errors_never_close);
    RUN_TEST(test_note_read_error_errors_within_window_mean_closed);
    RUN_TEST(test_note_read_error_just_past_the_window_does_not_close);
    RUN_TEST(test_note_read_error_slow_timeouts_never_close);
    RUN_TEST(test_note_read_error_window_restarts_after_slow_streak);
    RUN_TEST(test_note_key_read_resets_streak);
    RUN_TEST(test_note_key_read_restarts_the_window);
    RUN_TEST(test_note_read_error_clock_backwards_does_not_close);
    return test_harness_result();
}
