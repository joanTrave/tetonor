#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "../test_harness.h"
#include "../../src/game/game_timer.h"

#define TEST_START_TIME ((time_t)1000)
#define TEST_HUGE_JUMP ((time_t)1000000000)
#define TEST_OUT_OF_RANGE_SECONDS 400000
#define TEST_NEGATIVE_SECONDS (-5)
#define TEST_BUFFER_SIZE 16
#define TEST_CANARY 'Z'

#define ASSERT_FORMAT(seconds, expected)                                       \
    do                                                                         \
    {                                                                          \
        char harness_text[TETONOR_ELAPSED_TEXT_SIZE];                          \
        ASSERT_TRUE(format_elapsed_time((seconds), harness_text,               \
                                        sizeof harness_text));                 \
        ASSERT_TRUE(strcmp(harness_text, (expected)) == 0);                    \
    } while (0)

static void test_timer_constants(void)
{
    ASSERT_EQ_INT(TETONOR_TIMER_MAX_SECONDS, 359999);
    ASSERT_EQ_INT(TETONOR_ELAPSED_TEXT_SIZE, 9);
}

static void test_start_game_timer_starts_at_zero_running(void)
{
    GameTimer timer;
    memset(&timer, 0x5a, sizeof timer);
    start_game_timer(&timer, TEST_START_TIME);
    ASSERT_EQ_INT(timer.elapsed_seconds, 0);
    ASSERT_TRUE(timer.running);
    ASSERT_TRUE(timer.last_tick == TEST_START_TIME);
}

static void test_update_game_timer_no_time_elapsed_keeps_zero(void)
{
    GameTimer timer;
    start_game_timer(&timer, TEST_START_TIME);
    ASSERT_FALSE(
        update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_START_TIME));
    ASSERT_EQ_INT(timer.elapsed_seconds, 0);
    ASSERT_TRUE(timer.running);
}

static void test_update_game_timer_accumulates_seconds(void)
{
    GameTimer timer;
    start_game_timer(&timer, TEST_START_TIME);
    update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_START_TIME + 1);
    ASSERT_EQ_INT(timer.elapsed_seconds, 1);
    update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_START_TIME + 59);
    ASSERT_EQ_INT(timer.elapsed_seconds, 59);
    update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_START_TIME + 60);
    ASSERT_EQ_INT(timer.elapsed_seconds, 60);
    ASSERT_TRUE(timer.running);
}

static void test_update_game_timer_accumulates_across_one_long_gap(void)
{
    GameTimer timer;
    start_game_timer(&timer, TEST_START_TIME);
    update_game_timer(&timer, BOARD_RESULT_WRONG, TEST_START_TIME + 3725);
    ASSERT_EQ_INT(timer.elapsed_seconds, 3725);
}

static void test_update_game_timer_clock_backwards_never_decreases(void)
{
    GameTimer timer;
    start_game_timer(&timer, TEST_START_TIME + 100);
    update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_START_TIME + 110);
    ASSERT_EQ_INT(timer.elapsed_seconds, 10);
    update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_START_TIME + 40);
    ASSERT_EQ_INT(timer.elapsed_seconds, 10);
    update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_START_TIME + 45);
    ASSERT_EQ_INT(timer.elapsed_seconds, 15);
    ASSERT_TRUE(timer.running);
}

static void test_update_game_timer_unknown_time_never_decreases(void)
{
    GameTimer timer;
    start_game_timer(&timer, TEST_START_TIME);
    update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_START_TIME + 7);
    update_game_timer(&timer, BOARD_RESULT_PENDING, (time_t)-1);
    ASSERT_EQ_INT(timer.elapsed_seconds, 7);
    ASSERT_TRUE(timer.running);
}

static void test_update_game_timer_huge_jump_is_capped(void)
{
    GameTimer timer;
    start_game_timer(&timer, 0);
    update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_HUGE_JUMP);
    ASSERT_EQ_INT(timer.elapsed_seconds, TETONOR_TIMER_MAX_SECONDS);
    update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_HUGE_JUMP + 1);
    ASSERT_EQ_INT(timer.elapsed_seconds, TETONOR_TIMER_MAX_SECONDS);
    ASSERT_TRUE(timer.running);
}

static void test_update_game_timer_cap_is_reached_exactly(void)
{
    GameTimer timer;
    start_game_timer(&timer, 0);
    update_game_timer(&timer, BOARD_RESULT_PENDING,
                      TETONOR_TIMER_MAX_SECONDS - 1);
    ASSERT_EQ_INT(timer.elapsed_seconds, TETONOR_TIMER_MAX_SECONDS - 1);
    update_game_timer(&timer, BOARD_RESULT_PENDING, TETONOR_TIMER_MAX_SECONDS);
    ASSERT_EQ_INT(timer.elapsed_seconds, TETONOR_TIMER_MAX_SECONDS);
    update_game_timer(&timer, BOARD_RESULT_PENDING,
                      TETONOR_TIMER_MAX_SECONDS + 1);
    ASSERT_EQ_INT(timer.elapsed_seconds, TETONOR_TIMER_MAX_SECONDS);
}

static void test_update_game_timer_pending_and_wrong_keep_running(void)
{
    GameTimer timer;
    start_game_timer(&timer, TEST_START_TIME);
    ASSERT_FALSE(
        update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_START_TIME + 1));
    ASSERT_TRUE(timer.running);
    ASSERT_FALSE(
        update_game_timer(&timer, BOARD_RESULT_WRONG, TEST_START_TIME + 2));
    ASSERT_TRUE(timer.running);
    ASSERT_EQ_INT(timer.elapsed_seconds, 2);
}

static void test_update_game_timer_correct_stops_once(void)
{
    GameTimer timer;
    start_game_timer(&timer, TEST_START_TIME);
    ASSERT_TRUE(
        update_game_timer(&timer, BOARD_RESULT_CORRECT, TEST_START_TIME + 42));
    ASSERT_FALSE(timer.running);
    ASSERT_EQ_INT(timer.elapsed_seconds, 42);
    ASSERT_FALSE(
        update_game_timer(&timer, BOARD_RESULT_CORRECT, TEST_START_TIME + 99));
    ASSERT_EQ_INT(timer.elapsed_seconds, 42);
    ASSERT_FALSE(timer.running);
}

static void test_update_game_timer_correct_in_same_second_completes(void)
{
    GameTimer timer;
    start_game_timer(&timer, TEST_START_TIME);
    ASSERT_TRUE(
        update_game_timer(&timer, BOARD_RESULT_CORRECT, TEST_START_TIME));
    ASSERT_EQ_INT(timer.elapsed_seconds, 0);
    ASSERT_FALSE(timer.running);
}

static void test_update_game_timer_correct_counts_the_final_interval(void)
{
    GameTimer timer;
    start_game_timer(&timer, TEST_START_TIME);
    update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_START_TIME + 10);
    ASSERT_TRUE(
        update_game_timer(&timer, BOARD_RESULT_CORRECT, TEST_START_TIME + 25));
    ASSERT_EQ_INT(timer.elapsed_seconds, 25);
}

static void test_update_game_timer_stopped_ignores_time_and_result(void)
{
    GameTimer timer;
    start_game_timer(&timer, TEST_START_TIME);
    update_game_timer(&timer, BOARD_RESULT_CORRECT, TEST_START_TIME + 30);
    GameTimer stopped           = timer;
    const BoardResult results[] = {BOARD_RESULT_PENDING, BOARD_RESULT_WRONG,
                                   BOARD_RESULT_CORRECT};
    for (size_t i = 0; i < sizeof results / sizeof results[0]; i++)
    {
        ASSERT_FALSE(
            update_game_timer(&timer, results[i], TEST_START_TIME + 1000));
        ASSERT_EQ_INT(timer.elapsed_seconds, stopped.elapsed_seconds);
        ASSERT_FALSE(timer.running);
        ASSERT_TRUE(timer.last_tick == stopped.last_tick);
    }
}

static void test_start_game_timer_after_completion_restarts(void)
{
    GameTimer timer;
    start_game_timer(&timer, TEST_START_TIME);
    update_game_timer(&timer, BOARD_RESULT_CORRECT, TEST_START_TIME + 30);
    start_game_timer(&timer, TEST_START_TIME + 500);
    ASSERT_EQ_INT(timer.elapsed_seconds, 0);
    ASSERT_TRUE(timer.running);
    ASSERT_FALSE(
        update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_START_TIME + 505));
    ASSERT_EQ_INT(timer.elapsed_seconds, 5);
    ASSERT_TRUE(
        update_game_timer(&timer, BOARD_RESULT_CORRECT, TEST_START_TIME + 506));
    ASSERT_EQ_INT(timer.elapsed_seconds, 6);
}

static void test_start_game_timer_mid_game_resets_elapsed(void)
{
    GameTimer timer;
    start_game_timer(&timer, TEST_START_TIME);
    update_game_timer(&timer, BOARD_RESULT_WRONG, TEST_START_TIME + 90);
    start_game_timer(&timer, TEST_START_TIME + 90);
    ASSERT_EQ_INT(timer.elapsed_seconds, 0);
    update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_START_TIME + 91);
    ASSERT_EQ_INT(timer.elapsed_seconds, 1);
}

static void test_start_game_timer_starts_unpaused(void)
{
    GameTimer timer;
    memset(&timer, 0x5a, sizeof timer);
    start_game_timer(&timer, TEST_START_TIME);
    ASSERT_FALSE(timer.paused);
}

static void test_pause_game_timer_counts_time_up_to_the_pause(void)
{
    GameTimer timer;
    start_game_timer(&timer, 100);
    update_game_timer(&timer, BOARD_RESULT_PENDING, 103);
    pause_game_timer(&timer, 105);
    ASSERT_EQ_INT(timer.elapsed_seconds, 5);
    ASSERT_TRUE(timer.paused);
    ASSERT_TRUE(timer.running);
    ASSERT_TRUE(timer.last_tick == 105);
}

static void test_update_game_timer_while_paused_adds_nothing(void)
{
    GameTimer timer;
    start_game_timer(&timer, 100);
    pause_game_timer(&timer, 105);
    const time_t later[] = {106, 400, 100000};
    for (size_t i = 0; i < sizeof later / sizeof later[0]; i++)
    {
        ASSERT_FALSE(update_game_timer(&timer, BOARD_RESULT_PENDING, later[i]));
        ASSERT_EQ_INT(timer.elapsed_seconds, 5);
        ASSERT_TRUE(timer.paused);
        ASSERT_TRUE(timer.last_tick == 105);
    }
}

static void test_resume_game_timer_continues_from_the_pause(void)
{
    GameTimer timer;
    start_game_timer(&timer, 100);
    pause_game_timer(&timer, 105);
    resume_game_timer(&timer, 5000);
    ASSERT_FALSE(timer.paused);
    ASSERT_TRUE(timer.running);
    ASSERT_EQ_INT(timer.elapsed_seconds, 5);
    ASSERT_FALSE(update_game_timer(&timer, BOARD_RESULT_PENDING, 5003));
    ASSERT_EQ_INT(timer.elapsed_seconds, 8);
}

static void test_resume_game_timer_after_a_pause_longer_than_the_cap(void)
{
    GameTimer timer;
    start_game_timer(&timer, 0);
    pause_game_timer(&timer, 7);
    resume_game_timer(&timer, TEST_HUGE_JUMP);
    update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_HUGE_JUMP + 3);
    ASSERT_EQ_INT(timer.elapsed_seconds, 10);
}

static void test_pause_resume_cycles_count_only_played_time(void)
{
    GameTimer timer;
    start_game_timer(&timer, 0);
    update_game_timer(&timer, BOARD_RESULT_PENDING, 10);
    pause_game_timer(&timer, 10);
    resume_game_timer(&timer, 1000);
    pause_game_timer(&timer, 1010);
    ASSERT_EQ_INT(timer.elapsed_seconds, 20);
    resume_game_timer(&timer, 5000);
    update_game_timer(&timer, BOARD_RESULT_PENDING, 5030);
    ASSERT_EQ_INT(timer.elapsed_seconds, 50);
    pause_game_timer(&timer, 5030);
    resume_game_timer(&timer, 9000);
    update_game_timer(&timer, BOARD_RESULT_PENDING, 9001);
    ASSERT_EQ_INT(timer.elapsed_seconds, 51);
}

static void test_pause_game_timer_twice_keeps_the_first_pause(void)
{
    GameTimer timer;
    start_game_timer(&timer, 100);
    pause_game_timer(&timer, 105);
    pause_game_timer(&timer, 200);
    ASSERT_EQ_INT(timer.elapsed_seconds, 5);
    ASSERT_TRUE(timer.paused);
    ASSERT_TRUE(timer.last_tick == 105);
    resume_game_timer(&timer, 300);
    update_game_timer(&timer, BOARD_RESULT_PENDING, 301);
    ASSERT_EQ_INT(timer.elapsed_seconds, 6);
}

static void test_resume_game_timer_when_not_paused_keeps_time(void)
{
    GameTimer timer;
    start_game_timer(&timer, 100);
    resume_game_timer(&timer, 105);
    ASSERT_FALSE(timer.paused);
    update_game_timer(&timer, BOARD_RESULT_PENDING, 106);
    ASSERT_EQ_INT(timer.elapsed_seconds, 6);
}

static void test_pause_game_timer_clock_backwards_never_decreases(void)
{
    GameTimer timer;
    start_game_timer(&timer, 100);
    update_game_timer(&timer, BOARD_RESULT_PENDING, 105);
    pause_game_timer(&timer, 90);
    ASSERT_EQ_INT(timer.elapsed_seconds, 5);
    ASSERT_TRUE(timer.paused);
    resume_game_timer(&timer, 200);
    update_game_timer(&timer, BOARD_RESULT_PENDING, 202);
    ASSERT_EQ_INT(timer.elapsed_seconds, 7);
}

static void test_pause_game_timer_huge_gap_is_capped(void)
{
    GameTimer timer;
    start_game_timer(&timer, 0);
    pause_game_timer(&timer, TEST_HUGE_JUMP);
    ASSERT_EQ_INT(timer.elapsed_seconds, TETONOR_TIMER_MAX_SECONDS);
}

static void test_update_game_timer_correct_while_paused_does_not_complete(void)
{
    GameTimer timer;
    start_game_timer(&timer, 100);
    pause_game_timer(&timer, 105);
    ASSERT_FALSE(update_game_timer(&timer, BOARD_RESULT_CORRECT, 200));
    ASSERT_TRUE(timer.running);
    ASSERT_EQ_INT(timer.elapsed_seconds, 5);
    resume_game_timer(&timer, 300);
    ASSERT_TRUE(update_game_timer(&timer, BOARD_RESULT_CORRECT, 302));
    ASSERT_FALSE(timer.running);
    ASSERT_EQ_INT(timer.elapsed_seconds, 7);
    ASSERT_FALSE(update_game_timer(&timer, BOARD_RESULT_CORRECT, 400));
}

static void test_pause_completed_timer_stays_stopped(void)
{
    GameTimer timer;
    start_game_timer(&timer, 100);
    ASSERT_TRUE(update_game_timer(&timer, BOARD_RESULT_CORRECT, 110));
    ASSERT_EQ_INT(timer.elapsed_seconds, 10);
    pause_game_timer(&timer, 120);
    ASSERT_EQ_INT(timer.elapsed_seconds, 10);
    ASSERT_FALSE(timer.running);
    resume_game_timer(&timer, 200);
    ASSERT_FALSE(update_game_timer(&timer, BOARD_RESULT_CORRECT, 300));
    ASSERT_EQ_INT(timer.elapsed_seconds, 10);
    ASSERT_FALSE(timer.running);
}

static void test_start_game_timer_clears_pause(void)
{
    GameTimer timer;
    start_game_timer(&timer, 100);
    pause_game_timer(&timer, 105);
    start_game_timer(&timer, 500);
    ASSERT_FALSE(timer.paused);
    ASSERT_EQ_INT(timer.elapsed_seconds, 0);
    update_game_timer(&timer, BOARD_RESULT_PENDING, 502);
    ASSERT_EQ_INT(timer.elapsed_seconds, 2);
}

static void test_format_elapsed_time_boundaries(void)
{
    ASSERT_FORMAT(0, "00:00");
    ASSERT_FORMAT(1, "00:01");
    ASSERT_FORMAT(59, "00:59");
    ASSERT_FORMAT(60, "01:00");
    ASSERT_FORMAT(83, "01:23");
    ASSERT_FORMAT(3599, "59:59");
    ASSERT_FORMAT(3600, "1:00:00");
    ASSERT_FORMAT(3661, "1:01:01");
    ASSERT_FORMAT(36000, "10:00:00");
    ASSERT_FORMAT(TETONOR_TIMER_MAX_SECONDS, "99:59:59");
}

static void test_format_elapsed_time_clamps_out_of_range(void)
{
    ASSERT_FORMAT(TEST_NEGATIVE_SECONDS, "00:00");
    ASSERT_FORMAT(INT32_MIN, "00:00");
    ASSERT_FORMAT(TETONOR_TIMER_MAX_SECONDS + 1, "99:59:59");
    ASSERT_FORMAT(TEST_OUT_OF_RANGE_SECONDS, "99:59:59");
    ASSERT_FORMAT(INT32_MAX, "99:59:59");
}

static void test_format_elapsed_time_fits_the_declared_buffer_size(void)
{
    char text[TETONOR_ELAPSED_TEXT_SIZE];
    ASSERT_TRUE(
        format_elapsed_time(TETONOR_TIMER_MAX_SECONDS, text, sizeof text));
    ASSERT_EQ_INT(strlen(text), TETONOR_ELAPSED_TEXT_SIZE - 1);
}

static void test_format_elapsed_time_rejects_small_buffer(void)
{
    char text[TEST_BUFFER_SIZE];
    memset(text, TEST_CANARY, sizeof text);
    ASSERT_FALSE(format_elapsed_time(0, text, 5));
    ASSERT_EQ_INT(text[0], '\0');
    ASSERT_TRUE(format_elapsed_time(0, text, 6));
    ASSERT_TRUE(strcmp(text, "00:00") == 0);

    memset(text, TEST_CANARY, sizeof text);
    ASSERT_FALSE(format_elapsed_time(3600, text, 7));
    ASSERT_EQ_INT(text[0], '\0');
    ASSERT_TRUE(format_elapsed_time(3600, text, 8));
    ASSERT_TRUE(strcmp(text, "1:00:00") == 0);
}

static void test_format_elapsed_time_rejects_unusable_buffers(void)
{
    char text[TEST_BUFFER_SIZE];
    memset(text, TEST_CANARY, sizeof text);
    ASSERT_FALSE(format_elapsed_time(0, text, 0));
    ASSERT_EQ_INT(text[0], TEST_CANARY);
    ASSERT_FALSE(format_elapsed_time(0, text, 1));
    ASSERT_EQ_INT(text[0], '\0');
}

#define TEST_RESTORED_SECONDS 754
#define TEST_RESTORE_TIME ((time_t)5000)

static void test_restore_game_timer_sets_elapsed_running_from_now(void)
{
    GameTimer timer;
    memset(&timer, 0x5a, sizeof timer);
    restore_game_timer(&timer, TEST_RESTORED_SECONDS, TEST_RESTORE_TIME);
    ASSERT_EQ_INT(timer.elapsed_seconds, TEST_RESTORED_SECONDS);
    ASSERT_TRUE(timer.running);
    ASSERT_FALSE(timer.paused);
    ASSERT_TRUE(timer.last_tick == TEST_RESTORE_TIME);
}

static void test_restore_game_timer_clamps_both_ends(void)
{
    GameTimer timer;
    restore_game_timer(&timer, 0, TEST_RESTORE_TIME);
    ASSERT_EQ_INT(timer.elapsed_seconds, 0);
    restore_game_timer(&timer, TETONOR_TIMER_MAX_SECONDS, TEST_RESTORE_TIME);
    ASSERT_EQ_INT(timer.elapsed_seconds, TETONOR_TIMER_MAX_SECONDS);
    restore_game_timer(&timer, TETONOR_TIMER_MAX_SECONDS + 1,
                       TEST_RESTORE_TIME);
    ASSERT_EQ_INT(timer.elapsed_seconds, TETONOR_TIMER_MAX_SECONDS);
    restore_game_timer(&timer, INT32_MAX, TEST_RESTORE_TIME);
    ASSERT_EQ_INT(timer.elapsed_seconds, TETONOR_TIMER_MAX_SECONDS);
    restore_game_timer(&timer, TEST_NEGATIVE_SECONDS, TEST_RESTORE_TIME);
    ASSERT_EQ_INT(timer.elapsed_seconds, 0);
    restore_game_timer(&timer, INT32_MIN, TEST_RESTORE_TIME);
    ASSERT_EQ_INT(timer.elapsed_seconds, 0);
}

static void test_restore_game_timer_counts_on_from_the_restored_time(void)
{
    GameTimer timer;
    restore_game_timer(&timer, TEST_RESTORED_SECONDS, TEST_RESTORE_TIME);
    ASSERT_FALSE(update_game_timer(&timer, BOARD_RESULT_PENDING,
                                   TEST_RESTORE_TIME + 10));
    ASSERT_EQ_INT(timer.elapsed_seconds, TEST_RESTORED_SECONDS + 10);
    ASSERT_FALSE(
        update_game_timer(&timer, BOARD_RESULT_WRONG, TEST_RESTORE_TIME + 25));
    ASSERT_EQ_INT(timer.elapsed_seconds, TEST_RESTORED_SECONDS + 25);
}

static void test_restore_game_timer_ignores_a_clock_before_now(void)
{
    GameTimer timer;
    restore_game_timer(&timer, TEST_RESTORED_SECONDS, TEST_RESTORE_TIME);
    update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_RESTORE_TIME - 100);
    ASSERT_EQ_INT(timer.elapsed_seconds, TEST_RESTORED_SECONDS);
}

static void test_restore_game_timer_pauses_and_resumes_like_a_started_one(void)
{
    GameTimer timer;
    restore_game_timer(&timer, TEST_RESTORED_SECONDS, TEST_RESTORE_TIME);
    pause_game_timer(&timer, TEST_RESTORE_TIME + 5);
    ASSERT_TRUE(timer.paused);
    ASSERT_EQ_INT(timer.elapsed_seconds, TEST_RESTORED_SECONDS + 5);
    ASSERT_FALSE(update_game_timer(&timer, BOARD_RESULT_PENDING,
                                   TEST_RESTORE_TIME + 90));
    ASSERT_EQ_INT(timer.elapsed_seconds, TEST_RESTORED_SECONDS + 5);
    resume_game_timer(&timer, TEST_RESTORE_TIME + 100);
    ASSERT_FALSE(timer.paused);
    update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_RESTORE_TIME + 103);
    ASSERT_EQ_INT(timer.elapsed_seconds, TEST_RESTORED_SECONDS + 8);
}

static void test_restore_game_timer_completes_once_on_a_correct_answer(void)
{
    GameTimer timer;
    restore_game_timer(&timer, TEST_RESTORED_SECONDS, TEST_RESTORE_TIME);
    ASSERT_TRUE(update_game_timer(&timer, BOARD_RESULT_CORRECT,
                                  TEST_RESTORE_TIME + 20));
    ASSERT_EQ_INT(timer.elapsed_seconds, TEST_RESTORED_SECONDS + 20);
    ASSERT_FALSE(timer.running);
    ASSERT_FALSE(update_game_timer(&timer, BOARD_RESULT_CORRECT,
                                   TEST_RESTORE_TIME + 30));
}

static void test_restore_game_timer_revives_a_completed_timer(void)
{
    GameTimer timer;
    start_game_timer(&timer, TEST_START_TIME);
    ASSERT_TRUE(
        update_game_timer(&timer, BOARD_RESULT_CORRECT, TEST_START_TIME + 3));
    ASSERT_FALSE(timer.running);
    restore_game_timer(&timer, TEST_RESTORED_SECONDS, TEST_RESTORE_TIME);
    ASSERT_TRUE(timer.running);
    update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_RESTORE_TIME + 1);
    ASSERT_EQ_INT(timer.elapsed_seconds, TEST_RESTORED_SECONDS + 1);
}

static void test_restore_game_timer_at_the_cap_never_overflows(void)
{
    GameTimer timer;
    restore_game_timer(&timer, TETONOR_TIMER_MAX_SECONDS, TEST_RESTORE_TIME);
    update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_HUGE_JUMP);
    ASSERT_EQ_INT(timer.elapsed_seconds, TETONOR_TIMER_MAX_SECONDS);
}

int main(void)
{
    RUN_TEST(test_timer_constants);
    RUN_TEST(test_start_game_timer_starts_at_zero_running);
    RUN_TEST(test_update_game_timer_no_time_elapsed_keeps_zero);
    RUN_TEST(test_update_game_timer_accumulates_seconds);
    RUN_TEST(test_update_game_timer_accumulates_across_one_long_gap);
    RUN_TEST(test_update_game_timer_clock_backwards_never_decreases);
    RUN_TEST(test_update_game_timer_unknown_time_never_decreases);
    RUN_TEST(test_update_game_timer_huge_jump_is_capped);
    RUN_TEST(test_update_game_timer_cap_is_reached_exactly);
    RUN_TEST(test_update_game_timer_pending_and_wrong_keep_running);
    RUN_TEST(test_update_game_timer_correct_stops_once);
    RUN_TEST(test_update_game_timer_correct_in_same_second_completes);
    RUN_TEST(test_update_game_timer_correct_counts_the_final_interval);
    RUN_TEST(test_update_game_timer_stopped_ignores_time_and_result);
    RUN_TEST(test_start_game_timer_after_completion_restarts);
    RUN_TEST(test_start_game_timer_mid_game_resets_elapsed);
    RUN_TEST(test_start_game_timer_starts_unpaused);
    RUN_TEST(test_pause_game_timer_counts_time_up_to_the_pause);
    RUN_TEST(test_update_game_timer_while_paused_adds_nothing);
    RUN_TEST(test_resume_game_timer_continues_from_the_pause);
    RUN_TEST(test_resume_game_timer_after_a_pause_longer_than_the_cap);
    RUN_TEST(test_pause_resume_cycles_count_only_played_time);
    RUN_TEST(test_pause_game_timer_twice_keeps_the_first_pause);
    RUN_TEST(test_resume_game_timer_when_not_paused_keeps_time);
    RUN_TEST(test_pause_game_timer_clock_backwards_never_decreases);
    RUN_TEST(test_pause_game_timer_huge_gap_is_capped);
    RUN_TEST(test_update_game_timer_correct_while_paused_does_not_complete);
    RUN_TEST(test_pause_completed_timer_stays_stopped);
    RUN_TEST(test_start_game_timer_clears_pause);
    RUN_TEST(test_format_elapsed_time_boundaries);
    RUN_TEST(test_format_elapsed_time_clamps_out_of_range);
    RUN_TEST(test_format_elapsed_time_fits_the_declared_buffer_size);
    RUN_TEST(test_format_elapsed_time_rejects_small_buffer);
    RUN_TEST(test_format_elapsed_time_rejects_unusable_buffers);
    RUN_TEST(test_restore_game_timer_sets_elapsed_running_from_now);
    RUN_TEST(test_restore_game_timer_clamps_both_ends);
    RUN_TEST(test_restore_game_timer_counts_on_from_the_restored_time);
    RUN_TEST(test_restore_game_timer_ignores_a_clock_before_now);
    RUN_TEST(test_restore_game_timer_pauses_and_resumes_like_a_started_one);
    RUN_TEST(test_restore_game_timer_completes_once_on_a_correct_answer);
    RUN_TEST(test_restore_game_timer_revives_a_completed_timer);
    RUN_TEST(test_restore_game_timer_at_the_cap_never_overflows);
    return test_harness_result();
}
