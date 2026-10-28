#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../test_harness.h"
#include "../../src/game/daily_challenge.h"
#include "../../src/game/operand_group.h"

#define TEST_SEED_2026_10_27 20261027
#define TEST_SEED_1999_01_05 19990105
#define TEST_SEED_2000_12_31 20001231
#define TEST_EPOCH_2026_10_27 ((time_t)1793059200)
#define TEST_SECONDS_PER_DAY 86400
#define TEST_SENTINEL_YEAR (-999)
#define TEST_RAND_SEED 777
#define TEST_CONSECUTIVE_DAY_COUNT 10
#define TEST_CONSECUTIVE_FIRST_DAY 20
#define TEST_SHAPE_DAY_COUNT 9
#define TEST_UNDERSIZED_BUFFER (TETONOR_DATE_TEXT_SIZE - 1)

static CalendarDate make_date(int year, int month, int day)
{
    return (CalendarDate){.year = year, .month = month, .day = day};
}

/* Field by field: a daily puzzle's own struct may carry unspecified padding,
 * and the two sides being compared were not necessarily built the same way
 * (one may come from the real generator, the other from a fixture). */
static bool puzzles_equal(const NumberGeneration* first,
                          const NumberGeneration* second)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        int row = i / TETONOR_GRID_SIZE;
        int col = i % TETONOR_GRID_SIZE;
        if (first->sum_matrix[row][col] != second->sum_matrix[row][col] ||
            first->available_array[i] != second->available_array[i] ||
            first->solution_array[i] != second->solution_array[i] ||
            first->solution_groups[i].smaller !=
                second->solution_groups[i].smaller ||
            first->solution_groups[i].operation !=
                second->solution_groups[i].operation ||
            first->solution_groups[i].greater !=
                second->solution_groups[i].greater)
        {
            return false;
        }
    }
    return true;
}

static bool sum_matrices_equal(const NumberGeneration* first,
                               const NumberGeneration* second)
{
    return memcmp(first->sum_matrix, second->sum_matrix,
                  sizeof first->sum_matrix) == 0;
}

static int count_hidden(const NumberGeneration* puzzle)
{
    int hidden = 0;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (puzzle->available_array[i] == TETONOR_HIDDEN_NUMBER)
        {
            hidden++;
        }
    }
    return hidden;
}

static bool cell_matches_its_group(const NumberGeneration* puzzle, int cell)
{
    int row                   = cell / TETONOR_GRID_SIZE;
    int col                   = cell % TETONOR_GRID_SIZE;
    const OperandGroup* group = &puzzle->solution_groups[cell];
    int value                 = group->operation == GROUP_OPERATION_ADD
                                    ? group->smaller + group->greater
                                    : group->smaller * group->greater;
    return puzzle->sum_matrix[row][col] == value;
}

static void test_daily_challenge_seed_matches_yyyymmdd(void)
{
    ASSERT_EQ_INT(daily_challenge_seed(make_date(2026, 10, 27)),
                  TEST_SEED_2026_10_27);
    ASSERT_EQ_INT(daily_challenge_seed(make_date(1999, 1, 5)),
                  TEST_SEED_1999_01_05);
    ASSERT_EQ_INT(daily_challenge_seed(make_date(2000, 12, 31)),
                  TEST_SEED_2000_12_31);
}

static void test_daily_challenge_difficulty_cycles_by_day_of_month(void)
{
    const int days[]     = {27, 28, 29, 30, 31, 1, 3};
    const int expected[] = {
        DIFFICULTY_EASY,   DIFFICULTY_MEDIUM, DIFFICULTY_HARD, DIFFICULTY_EASY,
        DIFFICULTY_MEDIUM, DIFFICULTY_MEDIUM, DIFFICULTY_EASY};
    for (size_t i = 0; i < sizeof days / sizeof days[0]; i++)
    {
        CalendarDate date = make_date(2026, 10, days[i]);
        ASSERT_EQ_INT(daily_challenge_difficulty(date), expected[i]);
    }
}

static void test_utc_calendar_date_known_instant(void)
{
    CalendarDate date;
    bool ok = utc_calendar_date(TEST_EPOCH_2026_10_27, &date);
    ASSERT_TRUE(ok);
    ASSERT_EQ_INT(date.year, 2026);
    ASSERT_EQ_INT(date.month, 10);
    ASSERT_EQ_INT(date.day, 27);
}

static void test_utc_calendar_date_stays_within_the_day_until_midnight(void)
{
    CalendarDate date;
    bool ok = utc_calendar_date(
        TEST_EPOCH_2026_10_27 + TEST_SECONDS_PER_DAY - 1, &date);
    ASSERT_TRUE(ok);
    ASSERT_EQ_INT(date.year, 2026);
    ASSERT_EQ_INT(date.month, 10);
    ASSERT_EQ_INT(date.day, 27);
}

static void test_utc_calendar_date_rolls_over_at_midnight(void)
{
    CalendarDate date;
    bool ok =
        utc_calendar_date(TEST_EPOCH_2026_10_27 + TEST_SECONDS_PER_DAY, &date);
    ASSERT_TRUE(ok);
    ASSERT_EQ_INT(date.year, 2026);
    ASSERT_EQ_INT(date.month, 10);
    ASSERT_EQ_INT(date.day, 28);
}

static void test_utc_calendar_date_rejects_minus_one(void)
{
    CalendarDate date =
        make_date(TEST_SENTINEL_YEAR, TEST_SENTINEL_YEAR, TEST_SENTINEL_YEAR);
    bool ok = utc_calendar_date((time_t)-1, &date);
    ASSERT_FALSE(ok);
    ASSERT_EQ_INT(date.year, TEST_SENTINEL_YEAR);
    ASSERT_EQ_INT(date.month, TEST_SENTINEL_YEAR);
    ASSERT_EQ_INT(date.day, TEST_SENTINEL_YEAR);
}

static void
test_generate_daily_challenge_is_deterministic_for_the_same_date(void)
{
    CalendarDate date        = make_date(2026, 10, 27);
    NumberGeneration* first  = generate_daily_challenge(date);
    NumberGeneration* second = generate_daily_challenge(date);
    ASSERT_NOT_NULL(first);
    ASSERT_NOT_NULL(second);
    bool equal = puzzles_equal(first, second);
    free_number_generation(first);
    free_number_generation(second);
    ASSERT_TRUE(equal);
}

static void test_generate_daily_challenge_ignores_the_process_seed(void)
{
    CalendarDate date = make_date(2026, 10, 27);
    srand(1);
    NumberGeneration* first = generate_daily_challenge(date);
    srand(2);
    NumberGeneration* second = generate_daily_challenge(date);
    ASSERT_NOT_NULL(first);
    ASSERT_NOT_NULL(second);
    bool equal = puzzles_equal(first, second);
    free_number_generation(first);
    free_number_generation(second);
    ASSERT_TRUE(equal);
}

static void test_generate_daily_challenge_does_not_touch_rand(void)
{
    srand(TEST_RAND_SEED);
    int reference = rand();
    srand(TEST_RAND_SEED);
    NumberGeneration* puzzle =
        generate_daily_challenge(make_date(2026, 10, 27));
    ASSERT_NOT_NULL(puzzle);
    int after = rand();
    free_number_generation(puzzle);
    ASSERT_EQ_INT(after, reference);
}

/* 27 and 30 share a difficulty (27 % 3 == 30 % 3 == 0, Easy), so any
 * difference between them comes from the seed, not the shape. */
static void test_generate_daily_challenge_different_dates_differ(void)
{
    NumberGeneration* first = generate_daily_challenge(make_date(2026, 10, 27));
    NumberGeneration* second =
        generate_daily_challenge(make_date(2026, 10, 30));
    ASSERT_NOT_NULL(first);
    ASSERT_NOT_NULL(second);
    bool equal = puzzles_equal(first, second);
    free_number_generation(first);
    free_number_generation(second);
    ASSERT_FALSE(equal);
}

static void test_generate_daily_challenge_varies_across_consecutive_days(void)
{
    NumberGeneration* puzzles[TEST_CONSECUTIVE_DAY_COUNT];
    bool all_generated = true;
    for (int day = 0; day < TEST_CONSECUTIVE_DAY_COUNT; day++)
    {
        puzzles[day] = generate_daily_challenge(
            make_date(2026, 10, TEST_CONSECUTIVE_FIRST_DAY + day));
        all_generated = all_generated && puzzles[day] != NULL;
    }
    int distinct = 0;
    for (int i = 0; all_generated && i < TEST_CONSECUTIVE_DAY_COUNT; i++)
    {
        bool matches_earlier = false;
        for (int j = 0; j < i; j++)
        {
            matches_earlier =
                matches_earlier || sum_matrices_equal(puzzles[i], puzzles[j]);
        }
        if (!matches_earlier)
        {
            distinct++;
        }
    }
    for (int day = 0; day < TEST_CONSECUTIVE_DAY_COUNT; day++)
    {
        free_number_generation(puzzles[day]);
    }
    ASSERT_TRUE(all_generated);
    ASSERT_TRUE(distinct > 1);
}

static void test_generate_daily_challenge_shape_matches_its_difficulty(void)
{
    const int test_hidden_counts[DIFFICULTY_COUNT] = {4, 8, 12};
    for (int day = 1; day <= TEST_SHAPE_DAY_COUNT; day++)
    {
        CalendarDate date         = make_date(2026, 10, day);
        int difficulty            = daily_challenge_difficulty(date);
        DifficultyProfile profile = get_difficulty_profile(difficulty);
        NumberGeneration* puzzle  = generate_daily_challenge(date);
        ASSERT_NOT_NULL(puzzle);
        int hidden        = count_hidden(puzzle);
        bool all_in_range = true;
        bool all_match    = true;
        for (int i = 0; i < TETONOR_CELL_COUNT; i++)
        {
            int value = puzzle->solution_array[i];
            if (value < profile.min_value || value > profile.max_value)
            {
                all_in_range = false;
            }
            if (!cell_matches_its_group(puzzle, i))
            {
                all_match = false;
            }
        }
        free_number_generation(puzzle);
        ASSERT_EQ_INT(hidden, test_hidden_counts[difficulty]);
        ASSERT_TRUE(all_in_range);
        ASSERT_TRUE(all_match);
    }
}

static void test_format_calendar_date_writes_iso_text(void)
{
    char text[TETONOR_DATE_TEXT_SIZE];
    bool ok = format_calendar_date(make_date(2026, 10, 27), text, sizeof text);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(strcmp(text, "2026-10-27") == 0);

    bool other_ok =
        format_calendar_date(make_date(2026, 1, 5), text, sizeof text);
    ASSERT_TRUE(other_ok);
    ASSERT_TRUE(strcmp(text, "2026-01-05") == 0);
}

static void test_format_calendar_date_too_small_buffer_fails(void)
{
    char text[TEST_UNDERSIZED_BUFFER];
    memset(text, 'x', sizeof text);
    char before[TEST_UNDERSIZED_BUFFER];
    memcpy(before, text, sizeof text);
    bool ok = format_calendar_date(make_date(2026, 10, 27), text, sizeof text);
    ASSERT_FALSE(ok);
    ASSERT_TRUE(memcmp(text, before, sizeof text) == 0);
}

static void test_format_calendar_date_null_buffer_fails(void)
{
    ASSERT_FALSE(format_calendar_date(make_date(2026, 10, 27), NULL,
                                      TETONOR_DATE_TEXT_SIZE));
}

int main(void)
{
    RUN_TEST(test_daily_challenge_seed_matches_yyyymmdd);
    RUN_TEST(test_daily_challenge_difficulty_cycles_by_day_of_month);
    RUN_TEST(test_utc_calendar_date_known_instant);
    RUN_TEST(test_utc_calendar_date_stays_within_the_day_until_midnight);
    RUN_TEST(test_utc_calendar_date_rolls_over_at_midnight);
    RUN_TEST(test_utc_calendar_date_rejects_minus_one);
    RUN_TEST(test_generate_daily_challenge_is_deterministic_for_the_same_date);
    RUN_TEST(test_generate_daily_challenge_ignores_the_process_seed);
    RUN_TEST(test_generate_daily_challenge_does_not_touch_rand);
    RUN_TEST(test_generate_daily_challenge_different_dates_differ);
    RUN_TEST(test_generate_daily_challenge_varies_across_consecutive_days);
    RUN_TEST(test_generate_daily_challenge_shape_matches_its_difficulty);
    RUN_TEST(test_format_calendar_date_writes_iso_text);
    RUN_TEST(test_format_calendar_date_too_small_buffer_fails);
    RUN_TEST(test_format_calendar_date_null_buffer_fails);
    return test_harness_result();
}
