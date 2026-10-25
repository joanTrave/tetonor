#include <stdint.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/ui/difficulty_menu.h"
#include "../../src/ui/stats_menu.h"

#define TEST_LONG_SECONDS 3600
#define TEST_BEST_SECONDS 83
#define TEST_AVERAGE_SECONDS 125

static void test_build_stats_menu_row_without_games_shows_no_time(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    StatsMenuRow row;
    build_stats_menu_row(&statistics, DIFFICULTY_EASY, &row);
    ASSERT_TRUE(strcmp(row.label, "Easy") == 0);
    ASSERT_EQ_INT(row.completed_count, 0);
    ASSERT_TRUE(strcmp(row.best, "--:--") == 0);
    ASSERT_TRUE(strcmp(row.average, "--:--") == 0);
}

static void test_build_stats_menu_row_formats_best_and_average(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    statistics.by_difficulty[DIFFICULTY_MEDIUM] =
        (DifficultyStatistics){2, TEST_BEST_SECONDS, 2 * TEST_AVERAGE_SECONDS};
    StatsMenuRow row;
    build_stats_menu_row(&statistics, DIFFICULTY_MEDIUM, &row);
    ASSERT_TRUE(strcmp(row.label, "Medium") == 0);
    ASSERT_EQ_INT(row.completed_count, 2);
    ASSERT_TRUE(strcmp(row.best, "01:23") == 0);
    ASSERT_TRUE(strcmp(row.average, "02:05") == 0);
}

static void test_build_stats_menu_row_rounds_the_average(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    statistics.by_difficulty[DIFFICULTY_EASY] = (DifficultyStatistics){2, 1, 3};
    StatsMenuRow row;
    build_stats_menu_row(&statistics, DIFFICULTY_EASY, &row);
    ASSERT_TRUE(strcmp(row.best, "00:01") == 0);
    ASSERT_TRUE(strcmp(row.average, "00:02") == 0);
}

static void test_build_stats_menu_row_zero_second_best_is_a_time(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    statistics.by_difficulty[DIFFICULTY_HARD] = (DifficultyStatistics){1, 0, 0};
    StatsMenuRow row;
    build_stats_menu_row(&statistics, DIFFICULTY_HARD, &row);
    ASSERT_EQ_INT(row.completed_count, 1);
    ASSERT_TRUE(strcmp(row.best, "00:00") == 0);
    ASSERT_TRUE(strcmp(row.average, "00:00") == 0);
}

static void test_build_stats_menu_row_long_times(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    statistics.by_difficulty[DIFFICULTY_HARD] =
        (DifficultyStatistics){1, TEST_LONG_SECONDS, TEST_LONG_SECONDS};
    StatsMenuRow row;
    build_stats_menu_row(&statistics, DIFFICULTY_HARD, &row);
    ASSERT_TRUE(strcmp(row.best, "1:00:00") == 0);
    ASSERT_TRUE(strcmp(row.average, "1:00:00") == 0);
}

static void test_build_stats_menu_row_largest_values(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    statistics.by_difficulty[DIFFICULTY_HARD] = (DifficultyStatistics){
        TETONOR_STATS_MAX_COMPLETED, TETONOR_TIMER_MAX_SECONDS,
        (int64_t)TETONOR_STATS_MAX_COMPLETED * TETONOR_TIMER_MAX_SECONDS};
    StatsMenuRow row;
    build_stats_menu_row(&statistics, DIFFICULTY_HARD, &row);
    ASSERT_EQ_INT(row.completed_count, TETONOR_STATS_MAX_COMPLETED);
    ASSERT_TRUE(strcmp(row.best, "99:59:59") == 0);
    ASSERT_TRUE(strcmp(row.average, "99:59:59") == 0);
}

static void test_build_stats_menu_row_rows_are_independent(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    statistics.by_difficulty[DIFFICULTY_EASY] = (DifficultyStatistics){1, 5, 5};
    StatsMenuRow row;
    build_stats_menu_row(&statistics, DIFFICULTY_MEDIUM, &row);
    ASSERT_EQ_INT(row.completed_count, 0);
    ASSERT_TRUE(strcmp(row.best, "--:--") == 0);
    build_stats_menu_row(&statistics, DIFFICULTY_EASY, &row);
    ASSERT_EQ_INT(row.completed_count, 1);
    ASSERT_TRUE(strcmp(row.best, "00:05") == 0);
}

static void test_build_stats_menu_row_labels_per_difficulty(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    StatsMenuRow row;
    build_stats_menu_row(&statistics, DIFFICULTY_EASY, &row);
    ASSERT_TRUE(strcmp(row.label, "Easy") == 0);
    build_stats_menu_row(&statistics, DIFFICULTY_MEDIUM, &row);
    ASSERT_TRUE(strcmp(row.label, "Medium") == 0);
    build_stats_menu_row(&statistics, DIFFICULTY_HARD, &row);
    ASSERT_TRUE(strcmp(row.label, "Hard") == 0);
}

static void test_build_stats_menu_row_out_of_range_difficulty_clamps_label(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    StatsMenuRow row;
    build_stats_menu_row(&statistics, -1, &row);
    ASSERT_TRUE(strcmp(row.label, difficulty_menu_label(-1)) == 0);
    build_stats_menu_row(&statistics, DIFFICULTY_COUNT + 5, &row);
    ASSERT_TRUE(
        strcmp(row.label, difficulty_menu_label(DIFFICULTY_COUNT + 5)) == 0);
}

static void test_no_time_text_fits_elapsed_buffer(void)
{
    ASSERT_TRUE(sizeof TETONOR_NO_TIME_TEXT <= TETONOR_ELAPSED_TEXT_SIZE);
    ASSERT_TRUE(strcmp(TETONOR_NO_TIME_TEXT, "--:--") == 0);
}

int main(void)
{
    RUN_TEST(test_build_stats_menu_row_without_games_shows_no_time);
    RUN_TEST(test_build_stats_menu_row_formats_best_and_average);
    RUN_TEST(test_build_stats_menu_row_rounds_the_average);
    RUN_TEST(test_build_stats_menu_row_zero_second_best_is_a_time);
    RUN_TEST(test_build_stats_menu_row_long_times);
    RUN_TEST(test_build_stats_menu_row_largest_values);
    RUN_TEST(test_build_stats_menu_row_rows_are_independent);
    RUN_TEST(test_build_stats_menu_row_labels_per_difficulty);
    RUN_TEST(test_build_stats_menu_row_out_of_range_difficulty_clamps_label);
    RUN_TEST(test_no_time_text_fits_elapsed_buffer);
    return test_harness_result();
}
