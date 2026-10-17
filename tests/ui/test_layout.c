#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/game/number_generation.h"
#include "../../src/game/operand_group.h"
#include "../../src/ui/layout.h"

#define TEST_FIELD_COUNT 3
#define TEST_MESSAGE_ROWS 1
#define TEST_LAST_INDEX (TETONOR_CELL_COUNT - 1)
#define TEST_LAST_GRID_INDEX (TETONOR_GRID_SIZE - 1)
#define TEST_ENLARGE_BUFFER_SIZE 80

#define ASSERT_POINT(point, expected_y, expected_x)                            \
    do                                                                         \
    {                                                                          \
        LayoutPoint harness_point = (point);                                   \
        ASSERT_EQ_INT(harness_point.y, expected_y);                            \
        ASSERT_EQ_INT(harness_point.x, expected_x);                            \
    } while (0)

static const GroupField fixture_fields[TEST_FIELD_COUNT] = {
    GROUP_FIELD_SMALLER, GROUP_FIELD_OPERATION, GROUP_FIELD_GREATER};

static void test_layout_centered_offset_centres_content(void)
{
    ASSERT_EQ_INT(layout_centered_offset(50, 18), 16);
    ASSERT_EQ_INT(layout_centered_offset(10, 4), 3);
    ASSERT_EQ_INT(layout_centered_offset(11, 4), 3);
}

static void test_layout_centered_offset_equal_sizes_is_zero(void)
{
    ASSERT_EQ_INT(layout_centered_offset(7, 7), 0);
}

static void test_layout_centered_offset_clamps_oversized_content_to_zero(void)
{
    ASSERT_EQ_INT(layout_centered_offset(5, 9), 0);
}

static void test_layout_centered_window_origin_centres_menu(void)
{
    ASSERT_POINT(layout_centered_window_origin(24, 80, 7, 50), 8, 15);
}

static void test_layout_centered_window_origin_clamps_small_terminal(void)
{
    ASSERT_POINT(layout_centered_window_origin(5, 40, 7, 50), 0, 0);
}

static void test_layout_sum_cell_origin_first_and_last_cell(void)
{
    ASSERT_POINT(layout_sum_cell_origin(0, 0), 1, 2);
    ASSERT_POINT(layout_sum_cell_origin(3, 3), 13, 35);
}

static void test_layout_cell_block_is_nine_columns_wide(void)
{
    ASSERT_EQ_INT(LAYOUT_CELL_W, 9);
}

static void test_layout_sum_cell_origin_blocks_are_spaced_without_overlap(void)
{
    for (int row = 0; row < TETONOR_GRID_SIZE; row++)
    {
        for (int col = 0; col < TEST_LAST_GRID_INDEX; col++)
        {
            LayoutPoint here = layout_sum_cell_origin(row, col);
            LayoutPoint next = layout_sum_cell_origin(row, col + 1);
            ASSERT_EQ_INT(next.y, here.y);
            ASSERT_EQ_INT(next.x - (here.x + LAYOUT_CELL_W), LAYOUT_CELL_GAP_X);
        }
    }
    for (int col = 0; col < TETONOR_GRID_SIZE; col++)
    {
        for (int row = 0; row < TEST_LAST_GRID_INDEX; row++)
        {
            LayoutPoint here = layout_sum_cell_origin(row, col);
            LayoutPoint next = layout_sum_cell_origin(row + 1, col);
            ASSERT_EQ_INT(next.x, here.x);
            ASSERT_EQ_INT(next.y - here.y, LAYOUT_CELL_PITCH_Y);
        }
    }
}

static void test_layout_cell_rows_are_ordered_inside_one_pitch(void)
{
    ASSERT_TRUE(LAYOUT_CELL_VALUE_ROW > 0);
    ASSERT_TRUE(LAYOUT_CELL_VALUE_ROW < LAYOUT_CELL_SPLIT_ROW);
    ASSERT_TRUE(LAYOUT_CELL_SPLIT_ROW < LAYOUT_CELL_GROUP_ROW);
    ASSERT_TRUE(LAYOUT_CELL_GROUP_ROW < LAYOUT_CELL_PITCH_Y);
}

static void test_layout_grid_bottom_row_is_below_the_last_block_row(void)
{
    ASSERT_EQ_INT(layout_grid_bottom_row(), 17);
    ASSERT_EQ_INT(layout_grid_bottom_row(),
                  layout_sum_cell_origin(TEST_LAST_GRID_INDEX, 0).y +
                      LAYOUT_CELL_PITCH_Y);
}

static void test_layout_group_field_origin_of_first_cell(void)
{
    ASSERT_POINT(layout_group_field_origin(0, 0, GROUP_FIELD_SMALLER), 4, 3);
    ASSERT_POINT(layout_group_field_origin(0, 0, GROUP_FIELD_OPERATION), 4, 6);
    ASSERT_POINT(layout_group_field_origin(0, 0, GROUP_FIELD_GREATER), 4, 8);
}

static void test_layout_group_field_origin_of_last_cell(void)
{
    ASSERT_POINT(layout_group_field_origin(3, 3, GROUP_FIELD_SMALLER), 16, 36);
    ASSERT_POINT(layout_group_field_origin(3, 3, GROUP_FIELD_OPERATION), 16,
                 39);
    ASSERT_POINT(layout_group_field_origin(3, 3, GROUP_FIELD_GREATER), 16, 41);
}

static void test_layout_group_field_width_is_two_one_two(void)
{
    ASSERT_EQ_INT(layout_group_field_width(GROUP_FIELD_SMALLER), 2);
    ASSERT_EQ_INT(layout_group_field_width(GROUP_FIELD_OPERATION), 1);
    ASSERT_EQ_INT(layout_group_field_width(GROUP_FIELD_GREATER), 2);
}

static void test_layout_group_fields_lie_inside_their_block(void)
{
    for (int row = 0; row < TETONOR_GRID_SIZE; row++)
    {
        for (int col = 0; col < TETONOR_GRID_SIZE; col++)
        {
            LayoutPoint cell = layout_sum_cell_origin(row, col);
            for (int f = 0; f < TEST_FIELD_COUNT; f++)
            {
                LayoutPoint field =
                    layout_group_field_origin(row, col, fixture_fields[f]);
                int width = layout_group_field_width(fixture_fields[f]);
                ASSERT_EQ_INT(field.y, cell.y + LAYOUT_CELL_GROUP_ROW);
                ASSERT_TRUE(field.x > cell.x);
                ASSERT_TRUE(field.x + width - 1 < cell.x + LAYOUT_CELL_W - 1);
            }
        }
    }
}

static void test_layout_group_fields_are_separated_by_a_rule_column(void)
{
    for (int row = 0; row < TETONOR_GRID_SIZE; row++)
    {
        for (int col = 0; col < TETONOR_GRID_SIZE; col++)
        {
            for (int f = 0; f + 1 < TEST_FIELD_COUNT; f++)
            {
                LayoutPoint left =
                    layout_group_field_origin(row, col, fixture_fields[f]);
                LayoutPoint right =
                    layout_group_field_origin(row, col, fixture_fields[f + 1]);
                int left_width = layout_group_field_width(fixture_fields[f]);
                ASSERT_EQ_INT(right.x - (left.x + left_width), 1);
            }
        }
    }
}

static void test_layout_group_row_is_between_split_and_bottom_rules(void)
{
    LayoutPoint cell  = layout_sum_cell_origin(1, 2);
    LayoutPoint field = layout_group_field_origin(1, 2, GROUP_FIELD_SMALLER);
    ASSERT_TRUE(field.y > cell.y + LAYOUT_CELL_SPLIT_ROW);
    ASSERT_TRUE(field.y < cell.y + LAYOUT_CELL_PITCH_Y);
}

static void test_layout_available_label_origin_above_cells(void)
{
    ASSERT_POINT(layout_available_label_origin(), 18, 2);
    ASSERT_EQ_INT(layout_available_label_origin().y + 1,
                  layout_available_cell_origin(0).y);
}

static void test_layout_available_label_is_below_the_grid(void)
{
    ASSERT_TRUE(layout_available_label_origin().y > layout_grid_bottom_row());
}

static void test_layout_available_cell_origin_first_and_last(void)
{
    ASSERT_POINT(layout_available_cell_origin(0), 19, 2);
    ASSERT_POINT(layout_available_cell_origin(TEST_LAST_INDEX), 19, 62);
}

static void test_layout_available_cells_do_not_overlap(void)
{
    for (int i = 0; i < TEST_LAST_INDEX; i++)
    {
        LayoutPoint here = layout_available_cell_origin(i);
        LayoutPoint next = layout_available_cell_origin(i + 1);
        ASSERT_EQ_INT(next.y, here.y);
        ASSERT_TRUE(next.x - here.x >= LAYOUT_MINI_W);
    }
}

static void test_layout_confirm_button_origin_beside_available_row(void)
{
    LayoutPoint origin = layout_confirm_button_origin();
    LayoutPoint last   = layout_available_cell_origin(TEST_LAST_INDEX);
    ASSERT_EQ_INT(origin.y, layout_available_cell_origin(0).y);
    ASSERT_EQ_INT(origin.x, last.x + LAYOUT_MINI_W + LAYOUT_BUTTON_GAP_X);
    ASSERT_POINT(origin, 19, 68);
}

static void test_layout_confirm_button_width_fits_label(void)
{
    ASSERT_EQ_INT(LAYOUT_BUTTON_W, 11);
    ASSERT_TRUE(LAYOUT_BUTTON_W >= (int)strlen(LAYOUT_CONFIRM_LABEL) + 2);
}

static void test_layout_confirm_button_does_not_overlap_boxes_or_message(void)
{
    LayoutPoint button  = layout_confirm_button_origin();
    LayoutPoint message = layout_result_message_origin();
    LayoutPoint last    = layout_available_cell_origin(TEST_LAST_INDEX);
    ASSERT_TRUE(button.x > last.x + LAYOUT_MINI_W - 1);
    ASSERT_TRUE(button.y > layout_grid_bottom_row());
    ASSERT_TRUE(button.y + LAYOUT_BUTTON_H - 1 < message.y);
}

static void test_layout_restart_button_origin_below_confirm(void)
{
    LayoutPoint origin  = layout_restart_button_origin();
    LayoutPoint confirm = layout_confirm_button_origin();
    ASSERT_EQ_INT(origin.y, confirm.y + LAYOUT_BUTTON_H);
    ASSERT_EQ_INT(origin.x, confirm.x);
    ASSERT_POINT(origin, 22, 68);
}

static void test_layout_restart_button_width_fits_label(void)
{
    ASSERT_TRUE(strcmp(LAYOUT_RESTART_LABEL, "RESTART") == 0);
    ASSERT_TRUE(LAYOUT_BUTTON_W >= (int)strlen(LAYOUT_RESTART_LABEL) + 2);
}

static void
test_layout_restart_button_does_not_overlap_boxes_confirm_or_message(void)
{
    LayoutPoint restart = layout_restart_button_origin();
    LayoutPoint confirm = layout_confirm_button_origin();
    LayoutPoint message = layout_result_message_origin();
    LayoutPoint last    = layout_available_cell_origin(TEST_LAST_INDEX);
    ASSERT_TRUE(restart.y > confirm.y + LAYOUT_BUTTON_H - 1);
    ASSERT_TRUE(restart.y > last.y + LAYOUT_MINI_H - 1 ||
                restart.x > last.x + LAYOUT_MINI_W - 1);
    ASSERT_TRUE(restart.y + LAYOUT_BUTTON_H - 1 < message.y);
}

static void test_layout_result_message_origin_below_restart_button(void)
{
    LayoutPoint origin = layout_result_message_origin();
    ASSERT_EQ_INT(origin.y, layout_restart_button_origin().y + LAYOUT_BUTTON_H +
                                LAYOUT_MESSAGE_GAP);
    ASSERT_EQ_INT(origin.x, LAYOUT_ORIGIN_X);
    ASSERT_POINT(origin, 26, 2);
}

static void test_layout_result_message_is_on_the_last_minimum_row(void)
{
    LayoutPoint message = layout_result_message_origin();
    ASSERT_EQ_INT(message.y + TEST_MESSAGE_ROWS - 1, LAYOUT_MIN_ROWS - 1);
}

static void test_layout_result_message_is_below_the_available_row(void)
{
    LayoutPoint strip   = layout_available_cell_origin(0);
    LayoutPoint message = layout_result_message_origin();
    ASSERT_TRUE(message.y > strip.y + LAYOUT_MINI_H - 1);
}

static void test_layout_result_texts_are_unchanged(void)
{
    ASSERT_TRUE(strcmp(LAYOUT_CORRECT_MESSAGE, "CORRECT ANSWER") == 0);
    ASSERT_TRUE(strcmp(LAYOUT_WRONG_MESSAGE, "WRONG ANSWER") == 0);
    ASSERT_TRUE(LAYOUT_RESULT_MESSAGE_W >= (int)strlen(LAYOUT_CORRECT_MESSAGE));
    ASSERT_TRUE(LAYOUT_RESULT_MESSAGE_W >= (int)strlen(LAYOUT_WRONG_MESSAGE));
}

static void test_layout_undo_notice_text(void)
{
    ASSERT_TRUE(strcmp(LAYOUT_UNDO_EMPTY_MESSAGE, "Nothing to undo") == 0);
}

static void test_layout_undo_notice_origin_is_right_of_the_result_message(void)
{
    LayoutPoint message = layout_result_message_origin();
    LayoutPoint origin  = layout_undo_notice_origin();
    ASSERT_EQ_INT(origin.y, message.y);
    ASSERT_EQ_INT(origin.x,
                  message.x + LAYOUT_RESULT_MESSAGE_W + LAYOUT_NOTICE_GAP_X);
    ASSERT_POINT(origin, 26, 18);
}

static void test_layout_undo_notice_does_not_overlap_result_texts(void)
{
    LayoutPoint message = layout_result_message_origin();
    LayoutPoint origin  = layout_undo_notice_origin();
    ASSERT_TRUE(origin.x > message.x + (int)strlen(LAYOUT_CORRECT_MESSAGE));
    ASSERT_TRUE(origin.x > message.x + (int)strlen(LAYOUT_WRONG_MESSAGE));
}

static void test_layout_undo_notice_fits_the_minimum_terminal(void)
{
    LayoutPoint origin = layout_undo_notice_origin();
    ASSERT_TRUE(origin.x + (int)strlen(LAYOUT_UNDO_EMPTY_MESSAGE) - 1 <=
                LAYOUT_MIN_COLS - 1);
    ASSERT_TRUE(origin.y <= LAYOUT_MIN_ROWS - 1);
    ASSERT_EQ_INT(LAYOUT_MIN_ROWS, 27);
    ASSERT_EQ_INT(LAYOUT_MIN_COLS, 80);
}

static void test_layout_whole_board_fits_the_minimum_terminal(void)
{
    LayoutPoint last_block =
        layout_sum_cell_origin(TEST_LAST_GRID_INDEX, TEST_LAST_GRID_INDEX);
    LayoutPoint last_mini = layout_available_cell_origin(TEST_LAST_INDEX);
    LayoutPoint button    = layout_confirm_button_origin();
    LayoutPoint restart   = layout_restart_button_origin();
    LayoutPoint message   = layout_result_message_origin();
    int last_row          = message.y + TEST_MESSAGE_ROWS - 1;
    int last_col          = button.x + LAYOUT_BUTTON_W - 1;

    ASSERT_TRUE(layout_grid_bottom_row() <= LAYOUT_MIN_ROWS - 1);
    ASSERT_TRUE(last_block.x + LAYOUT_CELL_W - 1 <= LAYOUT_MIN_COLS - 1);
    ASSERT_TRUE(last_mini.y + LAYOUT_MINI_H - 1 <= LAYOUT_MIN_ROWS - 1);
    ASSERT_TRUE(last_mini.x + LAYOUT_MINI_W - 1 <= LAYOUT_MIN_COLS - 1);
    ASSERT_TRUE(button.y + LAYOUT_BUTTON_H - 1 <= LAYOUT_MIN_ROWS - 1);
    ASSERT_TRUE(restart.y + LAYOUT_BUTTON_H - 1 <= LAYOUT_MIN_ROWS - 1);
    ASSERT_TRUE(restart.x + LAYOUT_BUTTON_W - 1 <= LAYOUT_MIN_COLS - 1);
    ASSERT_TRUE(last_row <= LAYOUT_MIN_ROWS - 1);
    ASSERT_TRUE(last_col <= LAYOUT_MIN_COLS - 1);
}

static void test_layout_minimum_terminal_is_eighty_by_twenty_seven(void)
{
    ASSERT_EQ_INT(LAYOUT_MIN_ROWS, 27);
    ASSERT_EQ_INT(LAYOUT_MIN_COLS, 80);
}

static void test_layout_fits_accepts_the_minimum_terminal(void)
{
    ASSERT_TRUE(layout_fits(27, 80));
}

static void test_layout_fits_accepts_larger_terminals(void)
{
    ASSERT_TRUE(layout_fits(28, 80));
    ASSERT_TRUE(layout_fits(27, 81));
    ASSERT_TRUE(layout_fits(50, 200));
}

static void test_layout_fits_rejects_terminals_one_short(void)
{
    ASSERT_FALSE(layout_fits(26, 80));
    ASSERT_FALSE(layout_fits(27, 79));
}

static void test_layout_fits_rejects_the_former_minimum_height(void)
{
    ASSERT_FALSE(layout_fits(24, 80));
    ASSERT_FALSE(layout_fits(25, 80));
}

static void test_layout_fits_rejects_degenerate_sizes(void)
{
    ASSERT_FALSE(layout_fits(0, 0));
    ASSERT_FALSE(layout_fits(1, 200));
    ASSERT_FALSE(layout_fits(50, 1));
    ASSERT_FALSE(layout_fits(-1, -1));
}

static void test_layout_enlarge_message_names_the_minimum_size(void)
{
    char text[TEST_ENLARGE_BUFFER_SIZE];
    int length = snprintf(text, sizeof text, LAYOUT_ENLARGE_FORMAT,
                          LAYOUT_MIN_COLS, LAYOUT_MIN_ROWS);
    ASSERT_TRUE(length > 0 && length < (int)sizeof text);
    ASSERT_TRUE(strcmp(text, "Please enlarge the terminal to at least 80x27") ==
                0);
}

static void test_layout_difficulty_option_rows_follow_levels(void)
{
    ASSERT_EQ_INT(layout_difficulty_option_row(DIFFICULTY_EASY), 3);
    ASSERT_EQ_INT(layout_difficulty_option_row(DIFFICULTY_MEDIUM), 4);
    ASSERT_EQ_INT(layout_difficulty_option_row(DIFFICULTY_HARD), 5);
}

static void test_layout_difficulty_menu_fits_minimum_terminal(void)
{
    ASSERT_TRUE(LAYOUT_DIFFICULTY_MENU_H <= LAYOUT_MIN_ROWS);
    ASSERT_TRUE(LAYOUT_DIFFICULTY_MENU_W <= LAYOUT_MIN_COLS);
    ASSERT_TRUE(LAYOUT_DIFFICULTY_TITLE_ROW <
                layout_difficulty_option_row(DIFFICULTY_EASY));
    ASSERT_TRUE(layout_difficulty_option_row(DIFFICULTY_HARD) <
                LAYOUT_DIFFICULTY_CHOOSE_ROW);
    ASSERT_TRUE(LAYOUT_DIFFICULTY_CHOOSE_ROW < LAYOUT_DIFFICULTY_BACK_ROW);
    ASSERT_TRUE(LAYOUT_DIFFICULTY_BACK_ROW < LAYOUT_DIFFICULTY_MENU_H - 1);
}

static void test_layout_difficulty_title_row_is_below_the_top_border(void)
{
    ASSERT_TRUE(LAYOUT_DIFFICULTY_TITLE_ROW >= 1);
}

static void test_layout_difficulty_hints_fit_window_width(void)
{
    ASSERT_TRUE(LAYOUT_MENU_TEXT_COL +
                    (int)strlen("UP/DOWN to choose, SPACE or ENTER to start") <=
                LAYOUT_DIFFICULTY_MENU_W - 1);
    ASSERT_TRUE(LAYOUT_MENU_TEXT_COL + (int)strlen("Press ESC to go back") <=
                LAYOUT_DIFFICULTY_MENU_W - 1);
}

static void test_layout_difficulty_menu_centres_on_the_minimum_terminal(void)
{
    LayoutPoint origin = layout_centered_window_origin(
        LAYOUT_MIN_ROWS, LAYOUT_MIN_COLS, LAYOUT_DIFFICULTY_MENU_H,
        LAYOUT_DIFFICULTY_MENU_W);
    ASSERT_TRUE(origin.y >= 0);
    ASSERT_TRUE(origin.x >= 0);
    ASSERT_TRUE(origin.y + LAYOUT_DIFFICULTY_MENU_H <= LAYOUT_MIN_ROWS);
    ASSERT_TRUE(origin.x + LAYOUT_DIFFICULTY_MENU_W <= LAYOUT_MIN_COLS);
}

static void test_layout_timer_origin_value(void)
{
    ASSERT_POINT(layout_timer_origin(), 2, 48);
}

static void test_layout_timer_origin_is_on_the_first_cell_value_row(void)
{
    ASSERT_EQ_INT(layout_timer_origin().y,
                  LAYOUT_ORIGIN_Y + LAYOUT_CELL_VALUE_ROW);
}

static void test_layout_timer_fits_minimum_terminal(void)
{
    LayoutPoint origin = layout_timer_origin();
    ASSERT_EQ_INT(LAYOUT_MIN_ROWS, 27);
    ASSERT_EQ_INT(LAYOUT_MIN_COLS, 80);
    ASSERT_TRUE(origin.x + LAYOUT_TIMER_W <= LAYOUT_MIN_COLS);
    ASSERT_TRUE(origin.y < LAYOUT_MIN_ROWS);
    ASSERT_TRUE(origin.x >= 0);
    ASSERT_TRUE(origin.y >= 0);
}

static void test_layout_timer_does_not_overlap_board(void)
{
    LayoutPoint origin    = layout_timer_origin();
    LayoutPoint last_cell = layout_sum_cell_origin(0, TEST_LAST_GRID_INDEX);
    int last_grid_column  = last_cell.x + LAYOUT_CELL_W - 1;
    LayoutPoint confirm   = layout_confirm_button_origin();
    ASSERT_TRUE(origin.x > last_grid_column);
    ASSERT_TRUE(origin.y < layout_available_label_origin().y);
    ASSERT_TRUE(origin.x + LAYOUT_TIMER_W - 1 < confirm.x ||
                origin.y < confirm.y);
}

static void test_layout_timer_does_not_overlap_the_result_row(void)
{
    LayoutPoint origin = layout_timer_origin();
    ASSERT_TRUE(origin.y < layout_result_message_origin().y);
}

static void test_layout_timer_width_matches_label_and_text(void)
{
    ASSERT_EQ_INT(LAYOUT_TIMER_W, (int)strlen(LAYOUT_TIMER_LABEL) +
                                      TETONOR_ELAPSED_TEXT_SIZE - 1);
    ASSERT_TRUE(strcmp(LAYOUT_TIMER_LABEL, "Time: ") == 0);
}

static void test_layout_main_menu_rows_fit_window(void)
{
    ASSERT_TRUE(LAYOUT_MENU_TITLE_ROW >= 1);
    ASSERT_TRUE(LAYOUT_MENU_TITLE_ROW < LAYOUT_MENU_START_ROW);
    ASSERT_TRUE(LAYOUT_MENU_START_ROW < LAYOUT_MENU_STATS_ROW);
    ASSERT_TRUE(LAYOUT_MENU_STATS_ROW < LAYOUT_MENU_EXIT_ROW);
    ASSERT_TRUE(LAYOUT_MENU_EXIT_ROW < LAYOUT_MENU_H - 1);
    ASSERT_EQ_INT(LAYOUT_MENU_START_ROW, 3);
    ASSERT_EQ_INT(LAYOUT_MENU_STATS_ROW, 4);
    ASSERT_EQ_INT(LAYOUT_MENU_EXIT_ROW, 5);
    ASSERT_EQ_INT(LAYOUT_MENU_H, 7);
}

static void test_layout_main_menu_statistics_hint_fits_window_width(void)
{
    ASSERT_TRUE(LAYOUT_MENU_TEXT_COL +
                    (int)strlen("Press S to view statistics") <=
                LAYOUT_MENU_W - 1);
}

static void test_layout_stats_rows_follow_levels(void)
{
    ASSERT_EQ_INT(layout_stats_row(DIFFICULTY_EASY), 4);
    ASSERT_EQ_INT(layout_stats_row(DIFFICULTY_MEDIUM), 5);
    ASSERT_EQ_INT(layout_stats_row(DIFFICULTY_HARD), 6);
    ASSERT_EQ_INT(layout_stats_row(DIFFICULTY_EASY), LAYOUT_STATS_FIRST_ROW);
}

static void test_layout_stats_menu_rows_fit_window(void)
{
    ASSERT_EQ_INT(LAYOUT_STATS_MENU_H, 10);
    ASSERT_EQ_INT(LAYOUT_STATS_MENU_W, LAYOUT_MENU_W);
    /* Room for the column header between the title and the first level. */
    ASSERT_TRUE(LAYOUT_STATS_FIRST_ROW >= LAYOUT_MENU_TITLE_ROW + 3);
    /* Room for the back hint below the last level, above the border. */
    ASSERT_TRUE(layout_stats_row(DIFFICULTY_HARD) + 2 <
                LAYOUT_STATS_MENU_H - 1);
}

static void test_layout_stats_columns_do_not_overlap(void)
{
    const int time_width = TETONOR_ELAPSED_TEXT_SIZE - 1;
    ASSERT_TRUE(LAYOUT_STATS_COUNT_W >= (int)strlen("2147483647"));
    ASSERT_TRUE(LAYOUT_STATS_COUNT_W >= (int)strlen("Completed"));
    ASSERT_TRUE(time_width >= (int)strlen("Average"));
    ASSERT_TRUE(LAYOUT_MENU_TEXT_COL + (int)strlen("Difficulty") <
                LAYOUT_STATS_COUNT_COL);
    ASSERT_TRUE(LAYOUT_STATS_COUNT_COL + LAYOUT_STATS_COUNT_W <
                LAYOUT_STATS_BEST_COL);
    ASSERT_TRUE(LAYOUT_STATS_BEST_COL + time_width < LAYOUT_STATS_AVERAGE_COL);
}

static void test_layout_stats_columns_fit_window_width(void)
{
    const int time_width = TETONOR_ELAPSED_TEXT_SIZE - 1;
    ASSERT_TRUE(LAYOUT_STATS_AVERAGE_COL + time_width <=
                LAYOUT_STATS_MENU_W - 1);
    ASSERT_TRUE(LAYOUT_MENU_TEXT_COL + (int)strlen("Press ESC to go back") <=
                LAYOUT_STATS_MENU_W - 1);
    ASSERT_TRUE(LAYOUT_MENU_TEXT_COL + (int)strlen("Statistics") <=
                LAYOUT_STATS_MENU_W - 1);
}

static void test_layout_stats_menu_centres_on_the_minimum_terminal(void)
{
    LayoutPoint origin =
        layout_centered_window_origin(LAYOUT_MIN_ROWS, LAYOUT_MIN_COLS,
                                      LAYOUT_STATS_MENU_H, LAYOUT_STATS_MENU_W);
    ASSERT_TRUE(LAYOUT_STATS_MENU_H <= LAYOUT_MIN_ROWS);
    ASSERT_TRUE(LAYOUT_STATS_MENU_W <= LAYOUT_MIN_COLS);
    ASSERT_TRUE(origin.y >= 0);
    ASSERT_TRUE(origin.x >= 0);
    ASSERT_TRUE(origin.y + LAYOUT_STATS_MENU_H <= LAYOUT_MIN_ROWS);
    ASSERT_TRUE(origin.x + LAYOUT_STATS_MENU_W <= LAYOUT_MIN_COLS);
}

int main(void)
{
    RUN_TEST(test_layout_centered_offset_centres_content);
    RUN_TEST(test_layout_centered_offset_equal_sizes_is_zero);
    RUN_TEST(test_layout_centered_offset_clamps_oversized_content_to_zero);
    RUN_TEST(test_layout_centered_window_origin_centres_menu);
    RUN_TEST(test_layout_centered_window_origin_clamps_small_terminal);
    RUN_TEST(test_layout_sum_cell_origin_first_and_last_cell);
    RUN_TEST(test_layout_cell_block_is_nine_columns_wide);
    RUN_TEST(test_layout_sum_cell_origin_blocks_are_spaced_without_overlap);
    RUN_TEST(test_layout_cell_rows_are_ordered_inside_one_pitch);
    RUN_TEST(test_layout_grid_bottom_row_is_below_the_last_block_row);
    RUN_TEST(test_layout_group_field_origin_of_first_cell);
    RUN_TEST(test_layout_group_field_origin_of_last_cell);
    RUN_TEST(test_layout_group_field_width_is_two_one_two);
    RUN_TEST(test_layout_group_fields_lie_inside_their_block);
    RUN_TEST(test_layout_group_fields_are_separated_by_a_rule_column);
    RUN_TEST(test_layout_group_row_is_between_split_and_bottom_rules);
    RUN_TEST(test_layout_available_label_origin_above_cells);
    RUN_TEST(test_layout_available_label_is_below_the_grid);
    RUN_TEST(test_layout_available_cell_origin_first_and_last);
    RUN_TEST(test_layout_available_cells_do_not_overlap);
    RUN_TEST(test_layout_confirm_button_origin_beside_available_row);
    RUN_TEST(test_layout_confirm_button_width_fits_label);
    RUN_TEST(test_layout_confirm_button_does_not_overlap_boxes_or_message);
    RUN_TEST(test_layout_restart_button_origin_below_confirm);
    RUN_TEST(test_layout_restart_button_width_fits_label);
    RUN_TEST(
        test_layout_restart_button_does_not_overlap_boxes_confirm_or_message);
    RUN_TEST(test_layout_result_message_origin_below_restart_button);
    RUN_TEST(test_layout_result_message_is_on_the_last_minimum_row);
    RUN_TEST(test_layout_result_message_is_below_the_available_row);
    RUN_TEST(test_layout_result_texts_are_unchanged);
    RUN_TEST(test_layout_undo_notice_text);
    RUN_TEST(test_layout_undo_notice_origin_is_right_of_the_result_message);
    RUN_TEST(test_layout_undo_notice_does_not_overlap_result_texts);
    RUN_TEST(test_layout_undo_notice_fits_the_minimum_terminal);
    RUN_TEST(test_layout_whole_board_fits_the_minimum_terminal);
    RUN_TEST(test_layout_minimum_terminal_is_eighty_by_twenty_seven);
    RUN_TEST(test_layout_fits_accepts_the_minimum_terminal);
    RUN_TEST(test_layout_fits_accepts_larger_terminals);
    RUN_TEST(test_layout_fits_rejects_terminals_one_short);
    RUN_TEST(test_layout_fits_rejects_the_former_minimum_height);
    RUN_TEST(test_layout_fits_rejects_degenerate_sizes);
    RUN_TEST(test_layout_enlarge_message_names_the_minimum_size);
    RUN_TEST(test_layout_difficulty_option_rows_follow_levels);
    RUN_TEST(test_layout_difficulty_menu_fits_minimum_terminal);
    RUN_TEST(test_layout_difficulty_title_row_is_below_the_top_border);
    RUN_TEST(test_layout_difficulty_hints_fit_window_width);
    RUN_TEST(test_layout_difficulty_menu_centres_on_the_minimum_terminal);
    RUN_TEST(test_layout_timer_origin_value);
    RUN_TEST(test_layout_timer_origin_is_on_the_first_cell_value_row);
    RUN_TEST(test_layout_timer_fits_minimum_terminal);
    RUN_TEST(test_layout_timer_does_not_overlap_board);
    RUN_TEST(test_layout_timer_does_not_overlap_the_result_row);
    RUN_TEST(test_layout_timer_width_matches_label_and_text);
    RUN_TEST(test_layout_main_menu_rows_fit_window);
    RUN_TEST(test_layout_main_menu_statistics_hint_fits_window_width);
    RUN_TEST(test_layout_stats_rows_follow_levels);
    RUN_TEST(test_layout_stats_menu_rows_fit_window);
    RUN_TEST(test_layout_stats_columns_do_not_overlap);
    RUN_TEST(test_layout_stats_columns_fit_window_width);
    RUN_TEST(test_layout_stats_menu_centres_on_the_minimum_terminal);
    return test_harness_result();
}
