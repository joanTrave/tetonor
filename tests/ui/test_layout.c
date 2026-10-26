#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/game/number_generation.h"
#include "../../src/game/operand_group.h"
#include "../../src/ui/layout.h"
#include "../../src/ui/pause_menu.h"
#include "../../src/ui/theme_menu.h"
#include "../../src/ui/victory_menu.h"

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
    ASSERT_TRUE(LAYOUT_MENU_STATS_ROW < LAYOUT_MENU_THEME_ROW);
    ASSERT_TRUE(LAYOUT_MENU_THEME_ROW < LAYOUT_MENU_EXIT_ROW);
    ASSERT_TRUE(LAYOUT_MENU_EXIT_ROW < LAYOUT_MENU_H - 1);
    ASSERT_EQ_INT(LAYOUT_MENU_START_ROW, 3);
    ASSERT_EQ_INT(LAYOUT_MENU_STATS_ROW, 4);
    ASSERT_EQ_INT(LAYOUT_MENU_THEME_ROW, 5);
    ASSERT_EQ_INT(LAYOUT_MENU_EXIT_ROW, 6);
    ASSERT_EQ_INT(LAYOUT_MENU_H, 8);
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

static void test_layout_help_lines_texts(void)
{
    ASSERT_EQ_INT(LAYOUT_HELP_LINE_COUNT, 3);
    ASSERT_TRUE(strcmp(layout_help_line(0),
                       "Arrows move   0-9 type   + x * "
                       "operation   BACKSPACE delete") == 0);
    ASSERT_TRUE(strcmp(layout_help_line(1),
                       "ENTER/SPACE press button   U undo   P pause   T "
                       "theme") == 0);
    ASSERT_TRUE(strcmp(layout_help_line(2),
                       "N new game   M main menu   ESC leave without asking") ==
                0);
    ASSERT_TRUE(strcmp(layout_help_line(-1), "") == 0);
    ASSERT_TRUE(strcmp(layout_help_line(LAYOUT_HELP_LINE_COUNT), "") == 0);
}

static bool help_mentions(const char* text)
{
    for (int i = 0; i < LAYOUT_HELP_LINE_COUNT; i++)
    {
        if (strstr(layout_help_line(i), text) != NULL)
        {
            return true;
        }
    }
    return false;
}

static void test_layout_help_lines_name_every_shortcut(void)
{
    ASSERT_TRUE(help_mentions("P pause"));
    ASSERT_TRUE(help_mentions("N new game"));
    ASSERT_TRUE(help_mentions("M main menu"));
    ASSERT_TRUE(help_mentions("U undo"));
    ASSERT_TRUE(help_mentions("ESC leave"));
}

static void test_layout_help_lines_fit_their_width(void)
{
    ASSERT_EQ_INT(layout_help_width(), 64);
    for (int i = 0; i < LAYOUT_HELP_LINE_COUNT; i++)
    {
        ASSERT_TRUE(strlen(layout_help_line(i)) > 0);
        ASSERT_TRUE((int)strlen(layout_help_line(i)) <= layout_help_width());
    }
}

static void test_layout_help_line_origins(void)
{
    ASSERT_POINT(layout_help_line_origin(0), 22, 2);
    ASSERT_POINT(layout_help_line_origin(1), 23, 2);
    ASSERT_POINT(layout_help_line_origin(2), 24, 2);
    for (int i = 0; i < LAYOUT_HELP_LINE_COUNT; i++)
    {
        ASSERT_EQ_INT(layout_help_line_origin(i).y,
                      layout_restart_button_origin().y + i);
        ASSERT_EQ_INT(layout_help_line_origin(i).x, LAYOUT_ORIGIN_X);
    }
}

static void test_layout_help_does_not_overlap(void)
{
    int first_row = layout_help_line_origin(0).y;
    int last_row  = layout_help_line_origin(LAYOUT_HELP_LINE_COUNT - 1).y;
    int last_available_row =
        layout_available_cell_origin(TEST_LAST_INDEX).y + LAYOUT_MINI_H - 1;
    ASSERT_TRUE(first_row > last_available_row);
    ASSERT_TRUE(last_row < layout_result_message_origin().y);
    ASSERT_TRUE(last_row < layout_undo_notice_origin().y);
    ASSERT_TRUE(first_row > layout_timer_origin().y);
    for (int i = 0; i < LAYOUT_HELP_LINE_COUNT; i++)
    {
        int right_edge =
            layout_help_line_origin(i).x + (int)strlen(layout_help_line(i)) - 1;
        ASSERT_TRUE(right_edge < layout_restart_button_origin().x);
        ASSERT_TRUE(right_edge < layout_confirm_button_origin().x);
    }
}

static void test_layout_help_fits_the_minimum_terminal(void)
{
    ASSERT_EQ_INT(LAYOUT_MIN_ROWS, 27);
    ASSERT_EQ_INT(LAYOUT_MIN_COLS, 80);
    for (int i = 0; i < LAYOUT_HELP_LINE_COUNT; i++)
    {
        LayoutPoint origin = layout_help_line_origin(i);
        ASSERT_TRUE(origin.y <= LAYOUT_MIN_ROWS - 1);
        ASSERT_TRUE(origin.x + (int)strlen(layout_help_line(i)) - 1 <=
                    LAYOUT_MIN_COLS - 1);
    }
}

static void test_layout_pause_panel_fits(void)
{
    ASSERT_EQ_INT(layout_pause_option_row(PAUSE_OPTION_RESUME), 3);
    ASSERT_EQ_INT(layout_pause_option_row(PAUSE_OPTION_ABANDON), 4);
    const int rows[] = {LAYOUT_PAUSE_TITLE_ROW,
                        layout_pause_option_row(PAUSE_OPTION_RESUME),
                        layout_pause_option_row(PAUSE_OPTION_ABANDON),
                        LAYOUT_PAUSE_CHOOSE_ROW, LAYOUT_PAUSE_BACK_ROW};
    for (size_t i = 0; i < sizeof rows / sizeof rows[0]; i++)
    {
        ASSERT_TRUE(rows[i] >= 1);
        ASSERT_TRUE(rows[i] <= LAYOUT_PAUSE_MENU_H - 2);
        if (i > 0)
        {
            ASSERT_TRUE(rows[i] > rows[i - 1]);
        }
    }
    int text_width = LAYOUT_PAUSE_MENU_W - 2 * LAYOUT_MENU_TEXT_COL;
    ASSERT_TRUE((int)strlen(LAYOUT_PAUSE_CHOOSE_HINT) <= text_width);
    ASSERT_TRUE((int)strlen(LAYOUT_PAUSE_BACK_HINT) <= text_width);
    ASSERT_TRUE((int)strlen(LAYOUT_PAUSE_TITLE) <= text_width);
    for (int option = 0; option < PAUSE_OPTION_COUNT; option++)
    {
        ASSERT_TRUE((int)strlen("> ") +
                        (int)strlen(pause_option_label(option)) <=
                    text_width);
    }
    ASSERT_POINT(layout_centered_window_origin(LAYOUT_MIN_ROWS, LAYOUT_MIN_COLS,
                                               LAYOUT_PAUSE_MENU_H,
                                               LAYOUT_PAUSE_MENU_W),
                 9, 15);
}

static void test_layout_prompt_panel_fits(void)
{
    ASSERT_EQ_INT(layout_prompt_option_row(PROMPT_OPTION_YES), 4);
    ASSERT_EQ_INT(layout_prompt_option_row(PROMPT_OPTION_NO), 5);
    const int rows[] = {LAYOUT_PROMPT_TITLE_ROW,
                        LAYOUT_PROMPT_WARNING_ROW,
                        layout_prompt_option_row(PROMPT_OPTION_YES),
                        layout_prompt_option_row(PROMPT_OPTION_NO),
                        LAYOUT_PROMPT_CHOOSE_ROW,
                        LAYOUT_PROMPT_BACK_ROW};
    for (size_t i = 0; i < sizeof rows / sizeof rows[0]; i++)
    {
        ASSERT_TRUE(rows[i] >= 1);
        ASSERT_TRUE(rows[i] <= LAYOUT_PROMPT_H - 2);
        if (i > 0)
        {
            ASSERT_TRUE(rows[i] > rows[i - 1]);
        }
    }
    int text_width = LAYOUT_PROMPT_W - 2 * LAYOUT_MENU_TEXT_COL;
    ASSERT_TRUE((int)strlen(LAYOUT_PROMPT_WARNING) <= text_width);
    ASSERT_TRUE((int)strlen(LAYOUT_PROMPT_CHOOSE_HINT) <= text_width);
    ASSERT_TRUE((int)strlen(LAYOUT_PROMPT_BACK_HINT) <= text_width);
    ASSERT_TRUE((int)strlen(prompt_question(GAME_ACTION_NEW_GAME)) <=
                text_width);
    ASSERT_TRUE((int)strlen(prompt_question(GAME_ACTION_MAIN_MENU)) <=
                text_width);
    for (int option = 0; option < PROMPT_OPTION_COUNT; option++)
    {
        ASSERT_TRUE((int)strlen("> ") +
                        (int)strlen(prompt_option_label(option)) <=
                    text_width);
    }
    ASSERT_POINT(layout_centered_window_origin(LAYOUT_MIN_ROWS, LAYOUT_MIN_COLS,
                                               LAYOUT_PROMPT_H,
                                               LAYOUT_PROMPT_W),
                 8, 15);
}

static void test_layout_panels_do_not_cover_the_timer(void)
{
    LayoutPoint timer = layout_timer_origin();
    LayoutPoint pause =
        layout_centered_window_origin(LAYOUT_MIN_ROWS, LAYOUT_MIN_COLS,
                                      LAYOUT_PAUSE_MENU_H, LAYOUT_PAUSE_MENU_W);
    LayoutPoint prompt = layout_centered_window_origin(
        LAYOUT_MIN_ROWS, LAYOUT_MIN_COLS, LAYOUT_PROMPT_H, LAYOUT_PROMPT_W);
    ASSERT_TRUE(pause.y > timer.y);
    ASSERT_TRUE(prompt.y > timer.y);
}

static void test_layout_victory_rows(void)
{
    ASSERT_EQ_INT(layout_victory_summary_row(VICTORY_SUMMARY_TIME), 3);
    ASSERT_EQ_INT(layout_victory_summary_row(VICTORY_SUMMARY_DIFFICULTY), 4);
    ASSERT_EQ_INT(layout_victory_summary_row(VICTORY_SUMMARY_MOVES), 5);
    ASSERT_EQ_INT(layout_victory_option_row(VICTORY_OPTION_NEW_GAME), 7);
    ASSERT_EQ_INT(layout_victory_option_row(VICTORY_OPTION_MAIN_MENU), 8);
    ASSERT_EQ_INT(LAYOUT_VICTORY_TITLE_ROW, 1);
    ASSERT_EQ_INT(LAYOUT_VICTORY_CHOOSE_ROW, 10);
    ASSERT_EQ_INT(LAYOUT_VICTORY_SHORTCUT_ROW, 11);
    ASSERT_EQ_INT(LAYOUT_VICTORY_H, 13);
    ASSERT_EQ_INT(LAYOUT_VICTORY_W, LAYOUT_MENU_W);
}

static void test_layout_victory_panel_rows_inside(void)
{
    const int rows[] = {LAYOUT_VICTORY_TITLE_ROW,
                        layout_victory_summary_row(VICTORY_SUMMARY_TIME),
                        layout_victory_summary_row(VICTORY_SUMMARY_DIFFICULTY),
                        layout_victory_summary_row(VICTORY_SUMMARY_MOVES),
                        layout_victory_option_row(VICTORY_OPTION_NEW_GAME),
                        layout_victory_option_row(VICTORY_OPTION_MAIN_MENU),
                        LAYOUT_VICTORY_CHOOSE_ROW,
                        LAYOUT_VICTORY_SHORTCUT_ROW};
    for (size_t i = 0; i < sizeof rows / sizeof rows[0]; i++)
    {
        ASSERT_TRUE(rows[i] >= 1);
        ASSERT_TRUE(rows[i] <= LAYOUT_VICTORY_H - 2);
        if (i > 0)
        {
            ASSERT_TRUE(rows[i] > rows[i - 1]);
        }
    }
}

static void test_layout_victory_texts_fit(void)
{
    int text_width = LAYOUT_VICTORY_W - 2 * LAYOUT_MENU_TEXT_COL;
    ASSERT_EQ_INT(strcmp(LAYOUT_VICTORY_TITLE, "Puzzle solved!"), 0);
    ASSERT_EQ_INT(
        strcmp(LAYOUT_VICTORY_SHORTCUT_HINT, "N new game   M or ESC main menu"),
        0);
    ASSERT_EQ_INT(strcmp(LAYOUT_VICTORY_CHOOSE_HINT, LAYOUT_PAUSE_CHOOSE_HINT),
                  0);
    ASSERT_TRUE((int)strlen(LAYOUT_VICTORY_TITLE) <= text_width);
    ASSERT_TRUE((int)strlen(LAYOUT_VICTORY_CHOOSE_HINT) <= text_width);
    ASSERT_TRUE((int)strlen(LAYOUT_VICTORY_SHORTCUT_HINT) <= text_width);
    for (int option = 0; option < VICTORY_OPTION_COUNT; option++)
    {
        ASSERT_TRUE((int)strlen("> ") +
                        (int)strlen(victory_option_label(option)) <=
                    text_width);
    }
    VictorySummary longest;
    build_victory_summary(TETONOR_TIMER_MAX_SECONDS, DIFFICULTY_MEDIUM, INT_MAX,
                          &longest);
    for (int line = 0; line < VICTORY_SUMMARY_LINE_COUNT; line++)
    {
        ASSERT_TRUE(strlen(longest.lines[line]) > 0);
        ASSERT_TRUE((int)strlen(longest.lines[line]) <= text_width);
    }
}

static void test_layout_victory_fits_the_minimum_terminal(void)
{
    LayoutPoint origin = layout_centered_window_origin(
        LAYOUT_MIN_ROWS, LAYOUT_MIN_COLS, LAYOUT_VICTORY_H, LAYOUT_VICTORY_W);
    ASSERT_POINT(origin, 7, 15);
    ASSERT_EQ_INT(origin.y + LAYOUT_VICTORY_H - 1, 19);
    ASSERT_EQ_INT(origin.x + LAYOUT_VICTORY_W - 1, 64);
    ASSERT_TRUE(origin.y + LAYOUT_VICTORY_H - 1 <= LAYOUT_MIN_ROWS - 1);
    ASSERT_TRUE(origin.x + LAYOUT_VICTORY_W - 1 <= LAYOUT_MIN_COLS - 1);
    ASSERT_EQ_INT(LAYOUT_MIN_ROWS, 27);
    ASSERT_EQ_INT(LAYOUT_MIN_COLS, 80);
    ASSERT_TRUE(layout_fits(LAYOUT_MIN_ROWS, LAYOUT_MIN_COLS));
    ASSERT_FALSE(layout_fits(LAYOUT_MIN_ROWS - 1, LAYOUT_MIN_COLS));
}

static void test_layout_theme_rows(void)
{
    ASSERT_EQ_INT(PALETTE_THEME_COUNT, 2);
    ASSERT_EQ_INT(layout_theme_option_row(PALETTE_THEME_CLASSIC), 3);
    ASSERT_EQ_INT(layout_theme_option_row(PALETTE_THEME_LIGHT), 4);
    ASSERT_EQ_INT(LAYOUT_THEME_TITLE_ROW, 1);
    ASSERT_EQ_INT(LAYOUT_THEME_CHOOSE_ROW, 6);
    ASSERT_EQ_INT(LAYOUT_THEME_BACK_ROW, 7);
    ASSERT_EQ_INT(LAYOUT_THEME_MENU_H, 9);
    ASSERT_EQ_INT(LAYOUT_THEME_MENU_W, LAYOUT_MENU_W);
}

static void test_layout_theme_rows_follow_the_theme_count(void)
{
    int last_option = layout_theme_option_row(PALETTE_THEME_COUNT - 1);
    ASSERT_EQ_INT(layout_theme_option_row(0), LAYOUT_THEME_FIRST_OPTION_ROW);
    ASSERT_EQ_INT(LAYOUT_THEME_CHOOSE_ROW, last_option + 2);
    ASSERT_EQ_INT(LAYOUT_THEME_BACK_ROW, LAYOUT_THEME_CHOOSE_ROW + 1);
    ASSERT_EQ_INT(LAYOUT_THEME_MENU_H, LAYOUT_THEME_BACK_ROW + 2);
}

static void test_layout_theme_rows_inside(void)
{
    int rows[PALETTE_THEME_COUNT + 4];
    int count     = 0;
    rows[count++] = LAYOUT_THEME_TITLE_ROW;
    for (int theme = 0; theme < PALETTE_THEME_COUNT; theme++)
    {
        rows[count++] = layout_theme_option_row(theme);
    }
    rows[count++] = LAYOUT_THEME_CHOOSE_ROW;
    rows[count++] = LAYOUT_THEME_BACK_ROW;
    for (int i = 0; i < count; i++)
    {
        ASSERT_TRUE(rows[i] >= 1);
        ASSERT_TRUE(rows[i] <= LAYOUT_THEME_MENU_H - 2);
        if (i > 0)
        {
            ASSERT_TRUE(rows[i] > rows[i - 1]);
        }
    }
}

static void test_layout_theme_texts_fit(void)
{
    int text_width = LAYOUT_THEME_MENU_W - 2 * LAYOUT_MENU_TEXT_COL;
    ASSERT_EQ_INT(strcmp(LAYOUT_THEME_TITLE, "Choose theme"), 0);
    ASSERT_EQ_INT(strcmp(LAYOUT_THEME_BACK_HINT, "Press ESC or T to go back"),
                  0);
    ASSERT_EQ_INT(strcmp(LAYOUT_THEME_CHOOSE_HINT, LAYOUT_PAUSE_CHOOSE_HINT),
                  0);
    ASSERT_TRUE((int)strlen(LAYOUT_THEME_TITLE) <= text_width);
    ASSERT_TRUE((int)strlen(LAYOUT_THEME_CHOOSE_HINT) <= text_width);
    ASSERT_TRUE((int)strlen(LAYOUT_THEME_BACK_HINT) <= text_width);
    ThemeMenu menu;
    for (int active = 0; active < PALETTE_THEME_COUNT; active++)
    {
        init_theme_menu(&menu, active);
        for (int theme = 0; theme < PALETTE_THEME_COUNT; theme++)
        {
            int option_width = (int)strlen("> ") +
                               (int)strlen(theme_menu_label(theme)) +
                               (int)strlen(theme_menu_marker(&menu, theme));
            ASSERT_TRUE(option_width <= text_width);
        }
    }
}

static void test_layout_theme_fits_the_minimum_terminal(void)
{
    LayoutPoint origin =
        layout_centered_window_origin(LAYOUT_MIN_ROWS, LAYOUT_MIN_COLS,
                                      LAYOUT_THEME_MENU_H, LAYOUT_THEME_MENU_W);
    ASSERT_POINT(origin, 9, 15);
    ASSERT_EQ_INT(origin.y + LAYOUT_THEME_MENU_H - 1, 17);
    ASSERT_EQ_INT(origin.x + LAYOUT_THEME_MENU_W - 1, 64);
    ASSERT_TRUE(origin.y + LAYOUT_THEME_MENU_H - 1 <= LAYOUT_MIN_ROWS - 1);
    ASSERT_TRUE(origin.x + LAYOUT_THEME_MENU_W - 1 <= LAYOUT_MIN_COLS - 1);
    ASSERT_EQ_INT(LAYOUT_MIN_ROWS, 27);
    ASSERT_EQ_INT(LAYOUT_MIN_COLS, 80);
}

static void test_layout_main_menu_theme_hint_fits(void)
{
    ASSERT_TRUE(LAYOUT_MENU_TEXT_COL +
                    (int)strlen("Press T to choose a theme") <=
                LAYOUT_MENU_W - 1);
    ASSERT_TRUE(LAYOUT_MENU_THEME_ROW < LAYOUT_MENU_H - 1);
}

static void test_layout_pause_back_hint_mentions_theme(void)
{
    ASSERT_TRUE(strcmp(LAYOUT_PAUSE_BACK_HINT,
                       "Press ESC or P to resume, T for theme") == 0);
    ASSERT_TRUE((int)strlen(LAYOUT_PAUSE_BACK_HINT) <=
                LAYOUT_PAUSE_MENU_W - 2 * LAYOUT_MENU_TEXT_COL);
    ASSERT_EQ_INT(LAYOUT_PAUSE_MENU_H, 9);
}

static void test_layout_help_lines_name_theme_shortcut(void)
{
    ASSERT_TRUE(help_mentions("T theme"));
    ASSERT_TRUE(help_mentions("P pause"));
    ASSERT_TRUE(help_mentions("U undo"));
    for (int i = 0; i < LAYOUT_HELP_LINE_COUNT; i++)
    {
        ASSERT_TRUE((int)strlen(layout_help_line(i)) <= layout_help_width());
    }
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
    RUN_TEST(test_layout_help_lines_texts);
    RUN_TEST(test_layout_help_lines_name_every_shortcut);
    RUN_TEST(test_layout_help_lines_fit_their_width);
    RUN_TEST(test_layout_help_line_origins);
    RUN_TEST(test_layout_help_does_not_overlap);
    RUN_TEST(test_layout_help_fits_the_minimum_terminal);
    RUN_TEST(test_layout_pause_panel_fits);
    RUN_TEST(test_layout_prompt_panel_fits);
    RUN_TEST(test_layout_panels_do_not_cover_the_timer);
    RUN_TEST(test_layout_victory_rows);
    RUN_TEST(test_layout_victory_panel_rows_inside);
    RUN_TEST(test_layout_victory_texts_fit);
    RUN_TEST(test_layout_victory_fits_the_minimum_terminal);
    RUN_TEST(test_layout_theme_rows);
    RUN_TEST(test_layout_theme_rows_follow_the_theme_count);
    RUN_TEST(test_layout_theme_rows_inside);
    RUN_TEST(test_layout_theme_texts_fit);
    RUN_TEST(test_layout_theme_fits_the_minimum_terminal);
    RUN_TEST(test_layout_main_menu_theme_hint_fits);
    RUN_TEST(test_layout_pause_back_hint_mentions_theme);
    RUN_TEST(test_layout_help_lines_name_theme_shortcut);
    return test_harness_result();
}
