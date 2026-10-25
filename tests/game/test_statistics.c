#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/game/game_timer.h"
#include "../../src/game/statistics.h"

#define TEST_SERIALIZE_BUFFER_SIZE 512
#define TEST_AVERAGE_SENTINEL 777
#define TEST_PADDED_SIZE (TETONOR_STATS_TEXT_MAX + 1)

#define VALID_HEADER "TETONOR-STATS 1\n"
#define VALID_BODY "easy 0 0 0\nmedium 0 0 0\nhard 0 0 0\n"

typedef struct
{
    const char* name;
    const char* text;
    size_t length; /* 0 means strlen(text); set when the text holds a NUL */
} RejectedInput;

static const RejectedInput rejected_inputs[] = {
    {"empty", "", 0},
    {"header only", VALID_HEADER, 0},
    {"truncated mid-line", VALID_HEADER "easy 0 0 0\nmedium 0 0 0\nhard 0 0",
     0},
    {"truncated after key", VALID_HEADER "easy 0 0 0\nmedium 0 0 0\nhard", 0},
    {"missing hard line", VALID_HEADER "easy 0 0 0\nmedium 0 0 0\n", 0},
    {"extra line", VALID_HEADER VALID_BODY "extra 0 0 0\n", 0},
    {"blank line at the end", VALID_HEADER VALID_BODY "\n", 0},
    {"wrong order", VALID_HEADER "medium 0 0 0\neasy 0 0 0\nhard 0 0 0\n", 0},
    {"unknown key", VALID_HEADER "easy 0 0 0\nmedium 0 0 0\nexpert 0 0 0\n", 0},
    {"version 2", "TETONOR-STATS 2\n" VALID_BODY, 0},
    {"version 0", "TETONOR-STATS 0\n" VALID_BODY, 0},
    {"version 01", "TETONOR-STATS 01\n" VALID_BODY, 0},
    {"version 10", "TETONOR-STATS 10\n" VALID_BODY, 0},
    {"no version", "TETONOR-STATS\n" VALID_BODY, 0},
    {"header glued to version", "TETONOR-STATS1\n" VALID_BODY, 0},
    {"lowercase header", "tetonor-stats 1\n" VALID_BODY, 0},
    {"leading blank line", "\n" VALID_HEADER VALID_BODY, 0},
    {"trailing space after header", "TETONOR-STATS 1 \n" VALID_BODY, 0},
    {"negative count", VALID_HEADER "easy -1 0 0\nmedium 0 0 0\nhard 0 0 0\n",
     0},
    {"plus sign", VALID_HEADER "easy +1 0 0\nmedium 0 0 0\nhard 0 0 0\n", 0},
    {"decimal point", VALID_HEADER "easy 1.5 0 0\nmedium 0 0 0\nhard 0 0 0\n",
     0},
    {"letters", VALID_HEADER "easy a 0 0\nmedium 0 0 0\nhard 0 0 0\n", 0},
    {"hex digits", VALID_HEADER "easy 0x1 0 0\nmedium 0 0 0\nhard 0 0 0\n", 0},
    {"empty number", VALID_HEADER "easy  0 0\nmedium 0 0 0\nhard 0 0 0\n", 0},
    {"tab separator", VALID_HEADER "easy\t0 0 0\nmedium 0 0 0\nhard 0 0 0\n",
     0},
    {"double space", VALID_HEADER "easy 0  0 0\nmedium 0 0 0\nhard 0 0 0\n", 0},
    {"key glued to number",
     VALID_HEADER "easy0 0 0\nmedium 0 0 0\nhard 0 0 0\n", 0},
    {"trailing space on a line",
     VALID_HEADER "easy 0 0 0 \nmedium 0 0 0\nhard 0 0 0\n", 0},
    {"fourth number", VALID_HEADER "easy 0 0 0 0\nmedium 0 0 0\nhard 0 0 0\n",
     0},
    {"nineteen digits",
     VALID_HEADER "easy 0 0 0000000000000000000\nmedium 0 0 0\nhard 0 0 0\n",
     0},
    {"twenty digits",
     VALID_HEADER "easy 0 0 00000000000000000000\nmedium 0 0 0\nhard 0 0 0\n",
     0},
    {"twenty digits of nines",
     VALID_HEADER "easy 99999999999999999999 1 1\nmedium 0 0 0\nhard 0 0 0\n",
     0},
    {"zero games with a best time",
     VALID_HEADER "easy 0 1 0\nmedium 0 0 0\nhard 0 0 0\n", 0},
    {"zero games with a total",
     VALID_HEADER "easy 0 0 1\nmedium 0 0 0\nhard 0 0 0\n", 0},
    {"best above the cap",
     VALID_HEADER "easy 1 360000 360000\nmedium 0 0 0\nhard 0 0 0\n", 0},
    {"total below best times count",
     VALID_HEADER "easy 2 10 19\nmedium 0 0 0\nhard 0 0 0\n", 0},
    {"total above count times cap",
     VALID_HEADER "easy 1 5 360000\nmedium 0 0 0\nhard 0 0 0\n", 0},
    {"total above count times cap for many games",
     VALID_HEADER "easy 2 5 720000\nmedium 0 0 0\nhard 0 0 0\n", 0},
    {"count above INT32_MAX",
     VALID_HEADER "easy 2147483648 1 2147483648\nmedium 0 0 0\nhard 0 0 0\n",
     0},
    {"bad value on the last line",
     VALID_HEADER "easy 0 0 0\nmedium 0 0 0\nhard 1 5 4\n", 0},
    {"trailing garbage", VALID_HEADER VALID_BODY "x", 0},
    {"carriage return alone between lines",
     VALID_HEADER "easy 0 0 0\rmedium 0 0 0\rhard 0 0 0\r", 0},
    {"NUL inside a key", VALID_HEADER "easy 0 0 0\nmed\0um 0 0 0\nhard 0 0 0\n",
     sizeof(VALID_HEADER "easy 0 0 0\nmed\0um 0 0 0\nhard 0 0 0\n") - 1},
    {"NUL inside a number",
     VALID_HEADER "easy 0 0 0\nmedium 0 \0 0\nhard 0 0 0\n",
     sizeof(VALID_HEADER "easy 0 0 0\nmedium 0 \0 0\nhard 0 0 0\n") - 1},
    {"NUL counted after the last line", VALID_HEADER VALID_BODY "\0",
     sizeof(VALID_HEADER VALID_BODY "\0") - 1},
};

static Statistics make_sentinel_statistics(void)
{
    Statistics sentinel;
    memset(&sentinel, 0, sizeof sentinel);
    sentinel.by_difficulty[DIFFICULTY_EASY] = (DifficultyStatistics){7, 3, 21};
    sentinel.by_difficulty[DIFFICULTY_MEDIUM] = (DifficultyStatistics){1, 9, 9};
    sentinel.by_difficulty[DIFFICULTY_HARD] = (DifficultyStatistics){2, 4, 10};
    return sentinel;
}

static bool statistics_equal(const Statistics* left, const Statistics* right)
{
    for (int i = 0; i < DIFFICULTY_COUNT; i++)
    {
        if (left->by_difficulty[i].completed_count !=
                right->by_difficulty[i].completed_count ||
            left->by_difficulty[i].best_seconds !=
                right->by_difficulty[i].best_seconds ||
            left->by_difficulty[i].total_seconds !=
                right->by_difficulty[i].total_seconds)
        {
            return false;
        }
    }
    return true;
}

static void test_init_statistics_is_empty(void)
{
    Statistics statistics = make_sentinel_statistics();
    init_statistics(&statistics);
    for (int i = 0; i < DIFFICULTY_COUNT; i++)
    {
        ASSERT_EQ_INT(statistics.by_difficulty[i].completed_count, 0);
        ASSERT_EQ_INT(statistics.by_difficulty[i].best_seconds, 0);
        ASSERT_EQ_INT(statistics.by_difficulty[i].total_seconds, 0);
    }
}

static void test_statistics_constants(void)
{
    ASSERT_EQ_INT(TETONOR_STATS_MAX_COMPLETED, INT32_MAX);
    ASSERT_EQ_INT(TETONOR_STATS_TEXT_MAX, 256);
}

static void test_statistics_average_with_zero_games_reports_none(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    int32_t average = TEST_AVERAGE_SENTINEL;
    ASSERT_FALSE(statistics_average_seconds(
        &statistics.by_difficulty[DIFFICULTY_EASY], &average));
    ASSERT_EQ_INT(average, TEST_AVERAGE_SENTINEL);
}

static void test_record_completed_game_first_sets_best_and_average(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    ASSERT_TRUE(record_completed_game(&statistics, DIFFICULTY_EASY, 42));
    const DifficultyStatistics* easy =
        &statistics.by_difficulty[DIFFICULTY_EASY];
    ASSERT_EQ_INT(easy->completed_count, 1);
    ASSERT_EQ_INT(easy->best_seconds, 42);
    ASSERT_EQ_INT(easy->total_seconds, 42);
    int32_t average = 0;
    ASSERT_TRUE(statistics_average_seconds(easy, &average));
    ASSERT_EQ_INT(average, 42);
}

static void test_record_completed_game_first_game_of_zero_seconds(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    ASSERT_TRUE(record_completed_game(&statistics, DIFFICULTY_HARD, 0));
    const DifficultyStatistics* hard =
        &statistics.by_difficulty[DIFFICULTY_HARD];
    ASSERT_EQ_INT(hard->completed_count, 1);
    ASSERT_EQ_INT(hard->best_seconds, 0);
    ASSERT_TRUE(record_completed_game(&statistics, DIFFICULTY_HARD, 30));
    ASSERT_EQ_INT(hard->best_seconds, 0);
    ASSERT_EQ_INT(hard->total_seconds, 30);
}

static void test_record_completed_game_best_only_decreases(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    const DifficultyStatistics* medium =
        &statistics.by_difficulty[DIFFICULTY_MEDIUM];
    ASSERT_TRUE(record_completed_game(&statistics, DIFFICULTY_MEDIUM, 50));
    ASSERT_EQ_INT(medium->best_seconds, 50);
    ASSERT_TRUE(record_completed_game(&statistics, DIFFICULTY_MEDIUM, 30));
    ASSERT_EQ_INT(medium->best_seconds, 30);
    ASSERT_TRUE(record_completed_game(&statistics, DIFFICULTY_MEDIUM, 40));
    ASSERT_EQ_INT(medium->best_seconds, 30);
    ASSERT_TRUE(record_completed_game(&statistics, DIFFICULTY_MEDIUM, 30));
    ASSERT_EQ_INT(medium->best_seconds, 30);
    ASSERT_EQ_INT(medium->completed_count, 4);
}

static void test_statistics_average_over_several_games(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    record_completed_game(&statistics, DIFFICULTY_EASY, 10);
    record_completed_game(&statistics, DIFFICULTY_EASY, 20);
    record_completed_game(&statistics, DIFFICULTY_EASY, 31);
    const DifficultyStatistics* easy =
        &statistics.by_difficulty[DIFFICULTY_EASY];
    ASSERT_EQ_INT(easy->completed_count, 3);
    ASSERT_EQ_INT(easy->total_seconds, 61);
    int32_t average = 0;
    ASSERT_TRUE(statistics_average_seconds(easy, &average));
    ASSERT_EQ_INT(average, 20);
}

static int32_t average_of_two(int32_t first, int32_t second)
{
    Statistics statistics;
    init_statistics(&statistics);
    record_completed_game(&statistics, DIFFICULTY_EASY, first);
    record_completed_game(&statistics, DIFFICULTY_EASY, second);
    int32_t average = -1;
    statistics_average_seconds(&statistics.by_difficulty[DIFFICULTY_EASY],
                               &average);
    return average;
}

static void test_statistics_average_rounds_half_up(void)
{
    ASSERT_EQ_INT(average_of_two(1, 2), 2);
    ASSERT_EQ_INT(average_of_two(2, 3), 3);
    ASSERT_EQ_INT(average_of_two(0, 1), 1);
    ASSERT_EQ_INT(average_of_two(2, 2), 2);
    ASSERT_EQ_INT(average_of_two(0, 0), 0);
}

static void test_statistics_average_rounds_down_below_half(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    record_completed_game(&statistics, DIFFICULTY_EASY, 1);
    record_completed_game(&statistics, DIFFICULTY_EASY, 1);
    record_completed_game(&statistics, DIFFICULTY_EASY, 2);
    int32_t average = -1;
    ASSERT_TRUE(statistics_average_seconds(
        &statistics.by_difficulty[DIFFICULTY_EASY], &average));
    ASSERT_EQ_INT(average, 1);
}

static void test_record_completed_game_is_independent_per_difficulty(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    ASSERT_TRUE(record_completed_game(&statistics, DIFFICULTY_MEDIUM, 77));
    ASSERT_EQ_INT(statistics.by_difficulty[DIFFICULTY_MEDIUM].completed_count,
                  1);
    ASSERT_EQ_INT(statistics.by_difficulty[DIFFICULTY_MEDIUM].best_seconds, 77);
    ASSERT_EQ_INT(statistics.by_difficulty[DIFFICULTY_EASY].completed_count, 0);
    ASSERT_EQ_INT(statistics.by_difficulty[DIFFICULTY_EASY].total_seconds, 0);
    ASSERT_EQ_INT(statistics.by_difficulty[DIFFICULTY_HARD].completed_count, 0);
    ASSERT_EQ_INT(statistics.by_difficulty[DIFFICULTY_HARD].total_seconds, 0);
}

static void test_record_completed_game_rejects_bad_difficulty(void)
{
    Statistics statistics = make_sentinel_statistics();
    Statistics before     = statistics;
    ASSERT_FALSE(record_completed_game(&statistics, -1, 10));
    ASSERT_TRUE(statistics_equal(&statistics, &before));
    ASSERT_FALSE(record_completed_game(&statistics, DIFFICULTY_COUNT, 10));
    ASSERT_TRUE(statistics_equal(&statistics, &before));
    ASSERT_FALSE(record_completed_game(&statistics, INT32_MAX, 10));
    ASSERT_TRUE(statistics_equal(&statistics, &before));
}

static void test_record_completed_game_clamps_seconds(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    ASSERT_TRUE(record_completed_game(&statistics, DIFFICULTY_EASY, -7));
    ASSERT_EQ_INT(statistics.by_difficulty[DIFFICULTY_EASY].best_seconds, 0);
    ASSERT_EQ_INT(statistics.by_difficulty[DIFFICULTY_EASY].total_seconds, 0);
    ASSERT_TRUE(record_completed_game(&statistics, DIFFICULTY_HARD, INT32_MAX));
    ASSERT_EQ_INT(statistics.by_difficulty[DIFFICULTY_HARD].best_seconds,
                  TETONOR_TIMER_MAX_SECONDS);
    ASSERT_EQ_INT(statistics.by_difficulty[DIFFICULTY_HARD].total_seconds,
                  TETONOR_TIMER_MAX_SECONDS);
}

static void test_record_completed_game_accepts_the_cap_exactly(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    ASSERT_TRUE(record_completed_game(&statistics, DIFFICULTY_MEDIUM,
                                      TETONOR_TIMER_MAX_SECONDS));
    ASSERT_EQ_INT(statistics.by_difficulty[DIFFICULTY_MEDIUM].best_seconds,
                  TETONOR_TIMER_MAX_SECONDS);
}

static void test_record_completed_game_saturates_count(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    statistics.by_difficulty[DIFFICULTY_EASY] = (DifficultyStatistics){
        TETONOR_STATS_MAX_COMPLETED, 1, TETONOR_STATS_MAX_COMPLETED};
    Statistics before = statistics;
    ASSERT_FALSE(record_completed_game(&statistics, DIFFICULTY_EASY, 5));
    ASSERT_TRUE(statistics_equal(&statistics, &before));
    ASSERT_TRUE(record_completed_game(&statistics, DIFFICULTY_HARD, 5));
    ASSERT_EQ_INT(statistics.by_difficulty[DIFFICULTY_HARD].completed_count, 1);
}

static void
test_record_completed_game_accepts_the_last_game_before_saturation(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    statistics.by_difficulty[DIFFICULTY_EASY] = (DifficultyStatistics){
        TETONOR_STATS_MAX_COMPLETED - 1, 1, TETONOR_STATS_MAX_COMPLETED - 1};
    ASSERT_TRUE(record_completed_game(&statistics, DIFFICULTY_EASY, 1));
    ASSERT_EQ_INT(statistics.by_difficulty[DIFFICULTY_EASY].completed_count,
                  TETONOR_STATS_MAX_COMPLETED);
    ASSERT_EQ_INT(statistics.by_difficulty[DIFFICULTY_EASY].total_seconds,
                  TETONOR_STATS_MAX_COMPLETED);
}

static void test_statistics_large_totals_do_not_overflow(void)
{
    DifficultyStatistics entry = {
        TETONOR_STATS_MAX_COMPLETED, TETONOR_TIMER_MAX_SECONDS,
        (int64_t)TETONOR_STATS_MAX_COMPLETED * TETONOR_TIMER_MAX_SECONDS};
    int32_t average = 0;
    ASSERT_TRUE(statistics_average_seconds(&entry, &average));
    ASSERT_EQ_INT(average, TETONOR_TIMER_MAX_SECONDS);
}

static void test_serialize_statistics_exact_text(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    statistics.by_difficulty[DIFFICULTY_EASY] =
        (DifficultyStatistics){2, 10, 30};
    statistics.by_difficulty[DIFFICULTY_HARD] = (DifficultyStatistics){1, 5, 5};
    char text[TEST_SERIALIZE_BUFFER_SIZE];
    ASSERT_TRUE(serialize_statistics(&statistics, text, sizeof text));
    ASSERT_TRUE(strcmp(text, "TETONOR-STATS 1\n"
                             "easy 2 10 30\n"
                             "medium 0 0 0\n"
                             "hard 1 5 5\n") == 0);
}

static void test_serialize_statistics_empty_text(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    char text[TEST_SERIALIZE_BUFFER_SIZE];
    ASSERT_TRUE(serialize_statistics(&statistics, text, sizeof text));
    ASSERT_TRUE(strcmp(text, VALID_HEADER VALID_BODY) == 0);
}

static void test_serialize_statistics_extreme_text_fits_the_maximum(void)
{
    Statistics statistics;
    for (int i = 0; i < DIFFICULTY_COUNT; i++)
    {
        statistics.by_difficulty[i] = (DifficultyStatistics){
            TETONOR_STATS_MAX_COMPLETED, TETONOR_TIMER_MAX_SECONDS,
            (int64_t)TETONOR_STATS_MAX_COMPLETED * TETONOR_TIMER_MAX_SECONDS};
    }
    char text[TEST_SERIALIZE_BUFFER_SIZE];
    ASSERT_TRUE(serialize_statistics(&statistics, text, sizeof text));
    ASSERT_TRUE(strlen(text) <= TETONOR_STATS_TEXT_MAX);
}

static void test_serialize_statistics_rejects_small_buffer(void)
{
    Statistics statistics;
    init_statistics(&statistics);
    char text[TEST_SERIALIZE_BUFFER_SIZE];
    ASSERT_FALSE(serialize_statistics(&statistics, text, 10));
    ASSERT_FALSE(serialize_statistics(&statistics, text, 0));
    size_t length = strlen(VALID_HEADER VALID_BODY);
    ASSERT_FALSE(serialize_statistics(&statistics, text, length - 1));
    ASSERT_TRUE(serialize_statistics(&statistics, text, length + 1));
    ASSERT_TRUE(strcmp(text, VALID_HEADER VALID_BODY) == 0);
}

static void test_parse_statistics_round_trip(void)
{
    Statistics typical;
    init_statistics(&typical);
    typical.by_difficulty[DIFFICULTY_EASY] = (DifficultyStatistics){3, 12, 61};
    typical.by_difficulty[DIFFICULTY_MEDIUM] =
        (DifficultyStatistics){1, 3600, 3600};
    typical.by_difficulty[DIFFICULTY_HARD] = (DifficultyStatistics){2, 0, 90};

    Statistics extreme;
    for (int i = 0; i < DIFFICULTY_COUNT; i++)
    {
        extreme.by_difficulty[i] = (DifficultyStatistics){
            TETONOR_STATS_MAX_COMPLETED, TETONOR_TIMER_MAX_SECONDS,
            (int64_t)TETONOR_STATS_MAX_COMPLETED * TETONOR_TIMER_MAX_SECONDS};
    }

    Statistics empty;
    init_statistics(&empty);

    const Statistics* cases[] = {&empty, &typical, &extreme};
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++)
    {
        char text[TEST_SERIALIZE_BUFFER_SIZE];
        ASSERT_TRUE(serialize_statistics(cases[i], text, sizeof text));
        Statistics parsed = make_sentinel_statistics();
        ASSERT_TRUE(parse_statistics(text, strlen(text), &parsed));
        ASSERT_TRUE(statistics_equal(&parsed, cases[i]));
    }
}

static void test_parse_statistics_accepts_crlf_and_missing_final_newline(void)
{
    const char crlf[] = "TETONOR-STATS 1\r\neasy 2 10 30\r\nmedium 0 0 0\r\n"
                        "hard 1 5 5\r\n";
    const char bare[] = "TETONOR-STATS 1\neasy 2 10 30\nmedium 0 0 0\n"
                        "hard 1 5 5";
    Statistics expected;
    init_statistics(&expected);
    expected.by_difficulty[DIFFICULTY_EASY] = (DifficultyStatistics){2, 10, 30};
    expected.by_difficulty[DIFFICULTY_HARD] = (DifficultyStatistics){1, 5, 5};

    Statistics parsed                       = make_sentinel_statistics();
    ASSERT_TRUE(parse_statistics(crlf, sizeof crlf - 1, &parsed));
    ASSERT_TRUE(statistics_equal(&parsed, &expected));

    parsed = make_sentinel_statistics();
    ASSERT_TRUE(parse_statistics(bare, sizeof bare - 1, &parsed));
    ASSERT_TRUE(statistics_equal(&parsed, &expected));
}

static void test_parse_statistics_accepts_leading_zeros(void)
{
    const char text[] = "TETONOR-STATS 1\n"
                        "easy 000000000000000001 000000000000000001 "
                        "000000000000000001\n"
                        "medium 0 0 0\nhard 0 0 0\n";
    Statistics parsed = make_sentinel_statistics();
    ASSERT_TRUE(parse_statistics(text, sizeof text - 1, &parsed));
    ASSERT_EQ_INT(parsed.by_difficulty[DIFFICULTY_EASY].completed_count, 1);
    ASSERT_EQ_INT(parsed.by_difficulty[DIFFICULTY_EASY].best_seconds, 1);
    ASSERT_EQ_INT(parsed.by_difficulty[DIFFICULTY_EASY].total_seconds, 1);
}

static void test_parse_statistics_accepts_boundary_values(void)
{
    const char text[] = "TETONOR-STATS 1\n"
                        "easy 2 5 10\n"
                        "medium 1 359999 359999\n"
                        "hard 2 1 719998\n";
    Statistics parsed = make_sentinel_statistics();
    ASSERT_TRUE(parse_statistics(text, sizeof text - 1, &parsed));
    ASSERT_EQ_INT(parsed.by_difficulty[DIFFICULTY_EASY].total_seconds, 10);
    ASSERT_EQ_INT(parsed.by_difficulty[DIFFICULTY_MEDIUM].best_seconds,
                  TETONOR_TIMER_MAX_SECONDS);
    ASSERT_EQ_INT(parsed.by_difficulty[DIFFICULTY_HARD].total_seconds,
                  2 * TETONOR_TIMER_MAX_SECONDS);
}

static void test_parse_statistics_reads_only_the_given_length(void)
{
    const char text[]         = VALID_HEADER VALID_BODY "garbage";
    const size_t valid_length = sizeof(VALID_HEADER VALID_BODY) - 1;
    Statistics parsed         = make_sentinel_statistics();
    ASSERT_TRUE(parse_statistics(text, valid_length, &parsed));
    Statistics empty;
    init_statistics(&empty);
    ASSERT_TRUE(statistics_equal(&parsed, &empty));

    parsed = make_sentinel_statistics();
    ASSERT_FALSE(parse_statistics(text, valid_length - 4, &parsed));
}

static void test_parse_statistics_rejects_bad_input_and_keeps_output(void)
{
    const Statistics sentinel = make_sentinel_statistics();
    for (size_t i = 0; i < sizeof rejected_inputs / sizeof rejected_inputs[0];
         i++)
    {
        size_t length     = rejected_inputs[i].length != 0
                                ? rejected_inputs[i].length
                                : strlen(rejected_inputs[i].text);
        Statistics output = sentinel;
        if (parse_statistics(rejected_inputs[i].text, length, &output))
        {
            TEST_HARNESS_FAIL("accepted invalid input: %s",
                              rejected_inputs[i].name);
        }
        if (!statistics_equal(&output, &sentinel))
        {
            TEST_HARNESS_FAIL("modified the output for: %s",
                              rejected_inputs[i].name);
        }
    }
}

static void test_parse_statistics_rejects_text_over_the_maximum(void)
{
    char text[TEST_PADDED_SIZE];
    size_t valid_length = strlen(VALID_HEADER VALID_BODY);
    memset(text, '\n', sizeof text);
    memcpy(text, VALID_HEADER VALID_BODY, valid_length);
    const Statistics sentinel = make_sentinel_statistics();
    Statistics output         = sentinel;
    ASSERT_FALSE(parse_statistics(text, sizeof text, &output));
    ASSERT_TRUE(statistics_equal(&output, &sentinel));
}

int main(void)
{
    RUN_TEST(test_statistics_constants);
    RUN_TEST(test_init_statistics_is_empty);
    RUN_TEST(test_statistics_average_with_zero_games_reports_none);
    RUN_TEST(test_record_completed_game_first_sets_best_and_average);
    RUN_TEST(test_record_completed_game_first_game_of_zero_seconds);
    RUN_TEST(test_record_completed_game_best_only_decreases);
    RUN_TEST(test_statistics_average_over_several_games);
    RUN_TEST(test_statistics_average_rounds_half_up);
    RUN_TEST(test_statistics_average_rounds_down_below_half);
    RUN_TEST(test_record_completed_game_is_independent_per_difficulty);
    RUN_TEST(test_record_completed_game_rejects_bad_difficulty);
    RUN_TEST(test_record_completed_game_clamps_seconds);
    RUN_TEST(test_record_completed_game_accepts_the_cap_exactly);
    RUN_TEST(test_record_completed_game_saturates_count);
    RUN_TEST(
        test_record_completed_game_accepts_the_last_game_before_saturation);
    RUN_TEST(test_statistics_large_totals_do_not_overflow);
    RUN_TEST(test_serialize_statistics_exact_text);
    RUN_TEST(test_serialize_statistics_empty_text);
    RUN_TEST(test_serialize_statistics_extreme_text_fits_the_maximum);
    RUN_TEST(test_serialize_statistics_rejects_small_buffer);
    RUN_TEST(test_parse_statistics_round_trip);
    RUN_TEST(test_parse_statistics_accepts_crlf_and_missing_final_newline);
    RUN_TEST(test_parse_statistics_accepts_leading_zeros);
    RUN_TEST(test_parse_statistics_accepts_boundary_values);
    RUN_TEST(test_parse_statistics_reads_only_the_given_length);
    RUN_TEST(test_parse_statistics_rejects_bad_input_and_keeps_output);
    RUN_TEST(test_parse_statistics_rejects_text_over_the_maximum);
    return test_harness_result();
}
