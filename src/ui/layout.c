#include <stdio.h>

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
    "ENTER/SPACE press button   U undo   R redo   H hint   P pause",
    "N new game   M main menu   T theme   ESC leave without asking",
};

static const char* const uncounted_lines[LAYOUT_UNCOUNTED_LINE_COUNT] = {
    "Hint used: this game will not",
    "count for statistics.",
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

LayoutPoint layout_conflict_notice_origin(void)
{
    LayoutPoint origin = layout_undo_notice_origin();
    origin.x +=
        (int)(sizeof LAYOUT_UNDO_EMPTY_MESSAGE - 1) + LAYOUT_NOTICE_GAP_X;
    return origin;
}

void layout_format_conflict_notice(int count, char* text, size_t size)
{
    if (size == 0)
    {
        return;
    }
    if (count < 0)
    {
        count = 0;
    }
    if (count > TETONOR_CONFLICT_COUNT_MAX)
    {
        count = TETONOR_CONFLICT_COUNT_MAX;
    }
    snprintf(text, size, LAYOUT_CONFLICT_FORMAT, count, LAYOUT_CONFLICT_MARKER);
}

LayoutPoint layout_timer_origin(void)
{
    LayoutPoint last_cell = layout_sum_cell_origin(0, TETONOR_GRID_SIZE - 1);
    LayoutPoint origin;
    origin.y = LAYOUT_ORIGIN_Y + LAYOUT_CELL_VALUE_ROW;
    origin.x = last_cell.x + LAYOUT_CELL_W + LAYOUT_TIMER_GAP_X;
    return origin;
}

LayoutPoint layout_daily_badge_origin(void)
{
    LayoutPoint origin;
    origin.y = 0;
    origin.x = LAYOUT_ORIGIN_X;
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

LayoutPoint layout_hint_line_origin(int line)
{
    LayoutPoint origin = layout_timer_origin();
    origin.y += LAYOUT_SIDE_PANEL_GAP_Y + line;
    return origin;
}

LayoutPoint layout_uncounted_line_origin(int line)
{
    LayoutPoint origin = layout_hint_line_origin(LAYOUT_HINT_LINE_COUNT - 1);
    origin.y += 1 + LAYOUT_UNCOUNTED_GAP_Y + line;
    return origin;
}

int layout_side_panel_width(void)
{
    return LAYOUT_MIN_COLS - layout_timer_origin().x;
}

static char hint_operation_symbol(GroupOperation operation)
{
    return operation == GROUP_OPERATION_MULTIPLY ? LAYOUT_MULTIPLY_SYMBOL
                                                 : LAYOUT_ADD_SYMBOL;
}

static void format_suggestion_line(const BoardHint* hint, int line, char* text,
                                   size_t size)
{
    if (hint->target == HINT_TARGET_SLOT)
    {
        if (line == 0)
        {
            snprintf(text, size, "Hint: Available box %d", hint->index + 1);
        }
        else
        {
            snprintf(text, size, "enter %d", hint->value);
        }
        return;
    }
    if (line == 0)
    {
        snprintf(text, size, "Hint: row %d, column %d",
                 hint->index / TETONOR_GRID_SIZE + 1,
                 hint->index % TETONOR_GRID_SIZE + 1);
    }
    else
    {
        snprintf(text, size, "enter %d %c %d", hint->group.smaller,
                 hint_operation_symbol(hint->group.operation),
                 hint->group.greater);
    }
}

/* The two lines of a reply that has no suggestion. */
static void format_no_hint_line(HintStatus status, int line, char* text,
                                size_t size)
{
    const char* first  = "";
    const char* second = "";
    switch (status)
    {
        case HINT_STATUS_SOLVED:
            first = "No hint: puzzle already solved.";
            break;
        case HINT_STATUS_CONFLICTS:
            if (line == 0)
            {
                snprintf(text, size, LAYOUT_CONFLICT_HINT_FORMAT,
                         LAYOUT_CONFLICT_MARKER);
            }
            return;
        case HINT_STATUS_BOARD_FULL:
            first  = "No hint: every box is filled.";
            second = "Press CONFIRM or clear a box.";
            break;
        case HINT_STATUS_NO_SUGGESTION:
            first  = "No hint for these entries.";
            second = "Some may be wrong: try undo.";
            break;
        case HINT_STATUS_SUGGESTION:
            break;
    }
    snprintf(text, size, "%s", line == 0 ? first : second);
}

void layout_format_hint_line(const BoardHint* hint, int line, char* text,
                             size_t size)
{
    if (size == 0)
    {
        return;
    }
    text[0] = '\0';
    if (line < 0 || line >= LAYOUT_HINT_LINE_COUNT)
    {
        return;
    }
    if (hint->status == HINT_STATUS_SUGGESTION)
    {
        format_suggestion_line(hint, line, text, size);
        return;
    }
    format_no_hint_line(hint->status, line, text, size);
}

const char* layout_uncounted_line(int index)
{
    if (index < 0 || index >= LAYOUT_UNCOUNTED_LINE_COUNT)
    {
        return "";
    }
    return uncounted_lines[index];
}
