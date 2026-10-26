#include "layout.h"
#include "../game/game_port.h"

/* First inner column of each group box, relative to the block's left edge:
 * one rule, then smaller, rule, operation, rule, greater. */
#define SMALLER_OFFSET 1
#define OPERATION_OFFSET (SMALLER_OFFSET + LAYOUT_NUMBER_FIELD_W + 1)
#define GREATER_OFFSET (OPERATION_OFFSET + LAYOUT_OPERATION_FIELD_W + 1)

static const int group_field_offsets[] = {SMALLER_OFFSET, OPERATION_OFFSET,
                                          GREATER_OFFSET};

static const char* const help_lines[LAYOUT_HELP_LINE_COUNT] = {
    "Arrows move   0-9 type   + x * operation   BACKSPACE delete",
    "ENTER/SPACE press button   U undo   P pause   T theme",
    "N new game   M main menu   ESC leave without asking",
};

int layout_centered_offset(int container_size, int content_size)
{
    int offset = (container_size - content_size) / 2;
    return offset > 0 ? offset : 0;
}

LayoutPoint layout_centered_window_origin(int screen_rows, int screen_cols,
                                          int height, int width)
{
    LayoutPoint origin;
    origin.y = layout_centered_offset(screen_rows, height);
    origin.x = layout_centered_offset(screen_cols, width);
    return origin;
}

int layout_difficulty_option_row(int difficulty)
{
    return LAYOUT_DIFFICULTY_FIRST_OPTION_ROW + difficulty;
}

LayoutPoint layout_sum_cell_origin(int row, int col)
{
    LayoutPoint origin;
    origin.y = LAYOUT_ORIGIN_Y + row * LAYOUT_CELL_PITCH_Y;
    origin.x = LAYOUT_ORIGIN_X + col * (LAYOUT_CELL_W + LAYOUT_CELL_GAP_X);
    return origin;
}

int layout_grid_bottom_row(void)
{
    return LAYOUT_ORIGIN_Y + TETONOR_GRID_SIZE * LAYOUT_CELL_PITCH_Y;
}

LayoutPoint layout_group_field_origin(int row, int col, GroupField field)
{
    LayoutPoint origin = layout_sum_cell_origin(row, col);
    origin.y += LAYOUT_CELL_GROUP_ROW;
    origin.x += group_field_offsets[field];
    return origin;
}

int layout_group_field_width(GroupField field)
{
    return field == GROUP_FIELD_OPERATION ? LAYOUT_OPERATION_FIELD_W
                                          : LAYOUT_NUMBER_FIELD_W;
}

LayoutPoint layout_available_label_origin(void)
{
    LayoutPoint origin = layout_available_cell_origin(0);
    origin.y -= 1;
    return origin;
}

LayoutPoint layout_available_cell_origin(int index)
{
    LayoutPoint origin;
    origin.y = layout_grid_bottom_row() + 1 + LAYOUT_SECTION_GAP;
    origin.x = LAYOUT_ORIGIN_X + index * LAYOUT_MINI_W;
    return origin;
}

LayoutPoint layout_confirm_button_origin(void)
{
    LayoutPoint last   = layout_available_cell_origin(TETONOR_CELL_COUNT - 1);
    LayoutPoint origin = last;
    origin.x           = last.x + LAYOUT_MINI_W + LAYOUT_BUTTON_GAP_X;
    return origin;
}

LayoutPoint layout_restart_button_origin(void)
{
    LayoutPoint origin = layout_confirm_button_origin();
    origin.y += LAYOUT_BUTTON_H;
    return origin;
}

LayoutPoint layout_result_message_origin(void)
{
    LayoutPoint origin;
    origin.y =
        layout_restart_button_origin().y + LAYOUT_BUTTON_H + LAYOUT_MESSAGE_GAP;
    origin.x = LAYOUT_ORIGIN_X;
    return origin;
}

LayoutPoint layout_undo_notice_origin(void)
{
    LayoutPoint origin = layout_result_message_origin();
    origin.x += LAYOUT_RESULT_MESSAGE_W + LAYOUT_NOTICE_GAP_X;
    return origin;
}

LayoutPoint layout_timer_origin(void)
{
    LayoutPoint last_cell = layout_sum_cell_origin(0, TETONOR_GRID_SIZE - 1);
    LayoutPoint origin;
    origin.y = LAYOUT_ORIGIN_Y + LAYOUT_CELL_VALUE_ROW;
    origin.x = last_cell.x + LAYOUT_CELL_W + LAYOUT_TIMER_GAP_X;
    return origin;
}

const char* layout_help_line(int index)
{
    if (index < 0 || index >= LAYOUT_HELP_LINE_COUNT)
    {
        return "";
    }
    return help_lines[index];
}

LayoutPoint layout_help_line_origin(int index)
{
    LayoutPoint origin;
    origin.y = layout_restart_button_origin().y + index;
    origin.x = LAYOUT_ORIGIN_X;
    return origin;
}

int layout_help_width(void)
{
    return layout_confirm_button_origin().x - LAYOUT_BUTTON_GAP_X -
           LAYOUT_ORIGIN_X;
}

int layout_pause_option_row(int option)
{
    return LAYOUT_PAUSE_FIRST_OPTION_ROW + option;
}

int layout_prompt_option_row(int option)
{
    return LAYOUT_PROMPT_FIRST_OPTION_ROW + option;
}

int layout_victory_summary_row(int line)
{
    return LAYOUT_VICTORY_FIRST_SUMMARY_ROW + line;
}

int layout_victory_option_row(int option)
{
    return LAYOUT_VICTORY_FIRST_OPTION_ROW + option;
}

int layout_stats_row(int difficulty)
{
    return LAYOUT_STATS_FIRST_ROW + difficulty;
}

bool layout_fits(int rows, int cols)
{
    return rows >= LAYOUT_MIN_ROWS && cols >= LAYOUT_MIN_COLS;
}

int layout_theme_option_row(int theme)
{
    return LAYOUT_THEME_FIRST_OPTION_ROW + theme;
}
