#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/game/text_format.h"

#define TEST_MAX_DIGITS 18
#define TEST_NUMBER_CANARY 4242

static TextCursor cursor_over(const char* text)
{
    return make_text_cursor(text, strlen(text));
}

static void test_make_text_cursor_spans_the_text(void)
{
    const char* text  = "abc";
    TextCursor cursor = make_text_cursor(text, 3);
    ASSERT_TRUE(cursor.at == text);
    ASSERT_TRUE(cursor.end == text + 3);
    ASSERT_FALSE(is_text_cursor_at_end(&cursor));
}

static void test_make_text_cursor_empty_text_is_at_end(void)
{
    TextCursor cursor = make_text_cursor("", 0);
    ASSERT_TRUE(is_text_cursor_at_end(&cursor));
}

static void test_is_text_cursor_at_end_follows_consumption(void)
{
    TextCursor cursor = cursor_over("ab");
    ASSERT_TRUE(read_text_literal(&cursor, "a"));
    ASSERT_FALSE(is_text_cursor_at_end(&cursor));
    ASSERT_TRUE(read_text_literal(&cursor, "b"));
    ASSERT_TRUE(is_text_cursor_at_end(&cursor));
}

static void test_read_text_literal_match_advances(void)
{
    const char* text  = "TETONOR 1";
    TextCursor cursor = cursor_over(text);
    ASSERT_TRUE(read_text_literal(&cursor, "TETONOR"));
    ASSERT_TRUE(cursor.at == text + 7);
    ASSERT_TRUE(read_text_literal(&cursor, " 1"));
    ASSERT_TRUE(is_text_cursor_at_end(&cursor));
}

static void test_read_text_literal_mismatch_does_not_advance(void)
{
    const char* text  = "TETONOR 1";
    TextCursor cursor = cursor_over(text);
    ASSERT_FALSE(read_text_literal(&cursor, "TETONOX"));
    ASSERT_TRUE(cursor.at == text);
    ASSERT_FALSE(read_text_literal(&cursor, "tetonor"));
    ASSERT_TRUE(cursor.at == text);
}

static void test_read_text_literal_longer_than_the_text_fails(void)
{
    const char* text  = "abc";
    TextCursor cursor = cursor_over(text);
    ASSERT_FALSE(read_text_literal(&cursor, "abcd"));
    ASSERT_TRUE(cursor.at == text);
}

static void test_read_text_literal_stops_at_the_cursor_end(void)
{
    const char* text  = "abcdef";
    TextCursor cursor = make_text_cursor(text, 3);
    ASSERT_FALSE(read_text_literal(&cursor, "abcd"));
    ASSERT_TRUE(read_text_literal(&cursor, "abc"));
    ASSERT_TRUE(is_text_cursor_at_end(&cursor));
}

static void test_read_text_literal_at_end_fails(void)
{
    TextCursor cursor = cursor_over("");
    ASSERT_FALSE(read_text_literal(&cursor, "a"));
    ASSERT_TRUE(is_text_cursor_at_end(&cursor));
}

static void test_read_text_line_end_accepts_newline(void)
{
    const char* text  = "\nx";
    TextCursor cursor = cursor_over(text);
    ASSERT_TRUE(read_text_line_end(&cursor));
    ASSERT_TRUE(cursor.at == text + 1);
}

static void test_read_text_line_end_accepts_carriage_return_newline(void)
{
    const char* text  = "\r\nx";
    TextCursor cursor = cursor_over(text);
    ASSERT_TRUE(read_text_line_end(&cursor));
    ASSERT_TRUE(cursor.at == text + 2);
}

static void test_read_text_line_end_rejects_a_lone_carriage_return(void)
{
    const char* text  = "\rx";
    TextCursor cursor = cursor_over(text);
    ASSERT_FALSE(read_text_line_end(&cursor));
    ASSERT_TRUE(cursor.at == text);
    TextCursor last = cursor_over("\r");
    ASSERT_FALSE(read_text_line_end(&last));
    ASSERT_FALSE(is_text_cursor_at_end(&last));
}

static void test_read_text_line_end_rejects_other_text_and_the_end(void)
{
    const char* text  = "x\n";
    TextCursor cursor = cursor_over(text);
    ASSERT_FALSE(read_text_line_end(&cursor));
    ASSERT_TRUE(cursor.at == text);
    TextCursor empty = cursor_over("");
    ASSERT_FALSE(read_text_line_end(&empty));
}

static void test_read_text_number_reads_the_digit_run(void)
{
    const char* text  = "123 rest";
    TextCursor cursor = cursor_over(text);
    int64_t number    = TEST_NUMBER_CANARY;
    ASSERT_TRUE(read_text_number(&cursor, TEST_MAX_DIGITS, &number));
    ASSERT_EQ_INT(number, 123);
    ASSERT_TRUE(cursor.at == text + 3);
}

static void test_read_text_number_single_digit_and_zero(void)
{
    TextCursor cursor = cursor_over("0");
    int64_t number    = TEST_NUMBER_CANARY;
    ASSERT_TRUE(read_text_number(&cursor, TEST_MAX_DIGITS, &number));
    ASSERT_EQ_INT(number, 0);
    ASSERT_TRUE(is_text_cursor_at_end(&cursor));
}

static void test_read_text_number_accepts_leading_zeros(void)
{
    TextCursor cursor = cursor_over("007");
    int64_t number    = TEST_NUMBER_CANARY;
    ASSERT_TRUE(read_text_number(&cursor, TEST_MAX_DIGITS, &number));
    ASSERT_EQ_INT(number, 7);
    ASSERT_TRUE(is_text_cursor_at_end(&cursor));
}

static void test_read_text_number_accepts_exactly_max_digits(void)
{
    TextCursor cursor = cursor_over("1234");
    int64_t number    = TEST_NUMBER_CANARY;
    ASSERT_TRUE(read_text_number(&cursor, 4, &number));
    ASSERT_EQ_INT(number, 1234);
    ASSERT_TRUE(is_text_cursor_at_end(&cursor));
}

static void test_read_text_number_rejects_more_than_max_digits(void)
{
    const char* text  = "12345";
    TextCursor cursor = cursor_over(text);
    int64_t number    = TEST_NUMBER_CANARY;
    ASSERT_FALSE(read_text_number(&cursor, 4, &number));
    ASSERT_TRUE(cursor.at == text);
}

static void test_read_text_number_reads_eighteen_digits_exactly(void)
{
    TextCursor cursor = cursor_over("999999999999999999");
    int64_t number    = 0;
    ASSERT_TRUE(read_text_number(&cursor, TEST_MAX_DIGITS, &number));
    ASSERT_TRUE(number == INT64_C(999999999999999999));
    TextCursor longer = cursor_over("1000000000000000000");
    ASSERT_FALSE(read_text_number(&longer, TEST_MAX_DIGITS, &number));
}

static void test_read_text_number_rejects_non_digits(void)
{
    const char* inputs[] = {"x1", "-1", "+1", " 1", ".5", ""};
    for (size_t i = 0; i < sizeof inputs / sizeof inputs[0]; i++)
    {
        TextCursor cursor = cursor_over(inputs[i]);
        int64_t number    = TEST_NUMBER_CANARY;
        ASSERT_FALSE(read_text_number(&cursor, TEST_MAX_DIGITS, &number));
        ASSERT_TRUE(cursor.at == inputs[i]);
    }
}

static void test_read_text_number_stops_at_the_cursor_end(void)
{
    const char* text  = "98765";
    TextCursor cursor = make_text_cursor(text, 3);
    int64_t number    = TEST_NUMBER_CANARY;
    ASSERT_TRUE(read_text_number(&cursor, TEST_MAX_DIGITS, &number));
    ASSERT_EQ_INT(number, 987);
    ASSERT_TRUE(is_text_cursor_at_end(&cursor));
}

static void test_read_text_number_stops_at_a_non_digit_after_the_run(void)
{
    const char* text  = "12-3";
    TextCursor cursor = cursor_over(text);
    int64_t number    = TEST_NUMBER_CANARY;
    ASSERT_TRUE(read_text_number(&cursor, TEST_MAX_DIGITS, &number));
    ASSERT_EQ_INT(number, 12);
    ASSERT_TRUE(cursor.at == text + 2);
}

static void test_advance_text_output_adds_what_fits(void)
{
    size_t used = 0;
    ASSERT_TRUE(advance_text_output(5, 10, &used));
    ASSERT_EQ_INT(used, 5);
    ASSERT_TRUE(advance_text_output(4, 10, &used));
    ASSERT_EQ_INT(used, 9);
}

static void test_advance_text_output_zero_written_fits(void)
{
    size_t used = 3;
    ASSERT_TRUE(advance_text_output(0, 10, &used));
    ASSERT_EQ_INT(used, 3);
}

static void test_advance_text_output_needs_room_for_the_terminator(void)
{
    size_t used = 5;
    ASSERT_FALSE(advance_text_output(5, 10, &used));
    ASSERT_EQ_INT(used, 5);
    ASSERT_FALSE(advance_text_output(6, 10, &used));
    ASSERT_EQ_INT(used, 5);
    ASSERT_TRUE(advance_text_output(4, 10, &used));
    ASSERT_EQ_INT(used, 9);
}

static void test_advance_text_output_rejects_a_negative_count(void)
{
    size_t used = 2;
    ASSERT_FALSE(advance_text_output(-1, 10, &used));
    ASSERT_EQ_INT(used, 2);
}

static void test_advance_text_output_rejects_a_huge_count(void)
{
    size_t used = 0;
    ASSERT_FALSE(advance_text_output(1000000, 10, &used));
    ASSERT_EQ_INT(used, 0);
}

int main(void)
{
    RUN_TEST(test_make_text_cursor_spans_the_text);
    RUN_TEST(test_make_text_cursor_empty_text_is_at_end);
    RUN_TEST(test_is_text_cursor_at_end_follows_consumption);
    RUN_TEST(test_read_text_literal_match_advances);
    RUN_TEST(test_read_text_literal_mismatch_does_not_advance);
    RUN_TEST(test_read_text_literal_longer_than_the_text_fails);
    RUN_TEST(test_read_text_literal_stops_at_the_cursor_end);
    RUN_TEST(test_read_text_literal_at_end_fails);
    RUN_TEST(test_read_text_line_end_accepts_newline);
    RUN_TEST(test_read_text_line_end_accepts_carriage_return_newline);
    RUN_TEST(test_read_text_line_end_rejects_a_lone_carriage_return);
    RUN_TEST(test_read_text_line_end_rejects_other_text_and_the_end);
    RUN_TEST(test_read_text_number_reads_the_digit_run);
    RUN_TEST(test_read_text_number_single_digit_and_zero);
    RUN_TEST(test_read_text_number_accepts_leading_zeros);
    RUN_TEST(test_read_text_number_accepts_exactly_max_digits);
    RUN_TEST(test_read_text_number_rejects_more_than_max_digits);
    RUN_TEST(test_read_text_number_reads_eighteen_digits_exactly);
    RUN_TEST(test_read_text_number_rejects_non_digits);
    RUN_TEST(test_read_text_number_stops_at_the_cursor_end);
    RUN_TEST(test_read_text_number_stops_at_a_non_digit_after_the_run);
    RUN_TEST(test_advance_text_output_adds_what_fits);
    RUN_TEST(test_advance_text_output_zero_written_fits);
    RUN_TEST(test_advance_text_output_needs_room_for_the_terminator);
    RUN_TEST(test_advance_text_output_rejects_a_negative_count);
    RUN_TEST(test_advance_text_output_rejects_a_huge_count);
    return test_harness_result();
}
