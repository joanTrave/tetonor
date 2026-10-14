#include <curses.h>
#include <stdio.h>
#include <string.h>

#include "print_layout.h"
#include "layout.h"
#include "style.h"

#define NUMBER_BUFFER_SIZE 8
#define LAYOUT_ADD_SYMBOL '+'
#define LAYOUT_MULTIPLY_SYMBOL 'x'
#define BLOCK_INNER_W (LAYOUT_CELL_W - 2)
#define NO_SEPARATOR ((chtype)0)

static const char correct_message[] = "CORRECT ANSWER";
static const char wrong_message[]   = "WRONG ANSWER";

static void print_sum_matrix(const Board* board);
static void print_available_array(const Board* board);
static void print_confirm_button(const Board* board);
static void print_result(const Board* board);

void print_layout(const Board* board)
{
    int rows = 0;
    int cols = 0;
    getmaxyx(stdscr, rows, cols);
    if (!layout_fits(rows, cols))
    {
        mvprintw(0, 0, LAYOUT_ENLARGE_FORMAT, LAYOUT_MIN_COLS, LAYOUT_MIN_ROWS);
        return;
    }
    print_sum_matrix(board);
    print_available_array(board);
    print_confirm_button(board);
    print_result(board);
}

static void draw_box(int y, int x, int h, int w)
{
    mvaddch(y, x, ACS_ULCORNER);
    mvaddch(y, x + w - 1, ACS_URCORNER);
    mvaddch(y + h - 1, x, ACS_LLCORNER);
    mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);

    for (int i = 1; i < w - 1; i++)
    {
        mvaddch(y, x + i, ACS_HLINE);
        mvaddch(y + h - 1, x + i, ACS_HLINE);
    }

    for (int j = 1; j < h - 1; j++)
    {
        mvaddch(y + j, x, ACS_VLINE);
        mvaddch(y + j, x + w - 1, ACS_VLINE);
    }
}

static void draw_centered_number(int y, int x, int w, int value)
{
    char buf[NUMBER_BUFFER_SIZE];
    snprintf(buf, sizeof buf, "%d", value);

    int len = (int)strlen(buf);
    int px  = x + layout_centered_offset(w, len);

    mvprintw(y, px, "%s", buf);
}

/* Column offsets, from the block's left edge, of the two rules that split the
 * group row into three boxes. */
static int first_rule_offset(int row, int col)
{
    LayoutPoint block = layout_sum_cell_origin(row, col);
    LayoutPoint field =
        layout_group_field_origin(row, col, GROUP_FIELD_SMALLER);
    return field.x + layout_group_field_width(GROUP_FIELD_SMALLER) - block.x;
}

static int second_rule_offset(int row, int col)
{
    LayoutPoint block = layout_sum_cell_origin(row, col);
    LayoutPoint field =
        layout_group_field_origin(row, col, GROUP_FIELD_OPERATION);
    return field.x + layout_group_field_width(GROUP_FIELD_OPERATION) - block.x;
}

/* One horizontal rule across a block. separator, when set, joins the two rules
 * of the group row. */
static void draw_rule(int y, int row, int col, chtype left, chtype separator,
                      chtype right)
{
    LayoutPoint block = layout_sum_cell_origin(row, col);
    for (int i = 1; i < LAYOUT_CELL_W - 1; i++)
    {
        mvaddch(y, block.x + i, ACS_HLINE);
    }
    mvaddch(y, block.x, left);
    mvaddch(y, block.x + LAYOUT_CELL_W - 1, right);
    if (separator != NO_SEPARATOR)
    {
        mvaddch(y, block.x + first_rule_offset(row, col), separator);
        mvaddch(y, block.x + second_rule_offset(row, col), separator);
    }
}

static void draw_vertical_rules(int y, int row, int col, bool with_separators)
{
    LayoutPoint block = layout_sum_cell_origin(row, col);
    mvaddch(y, block.x, ACS_VLINE);
    mvaddch(y, block.x + LAYOUT_CELL_W - 1, ACS_VLINE);
    if (with_separators)
    {
        mvaddch(y, block.x + first_rule_offset(row, col), ACS_VLINE);
        mvaddch(y, block.x + second_rule_offset(row, col), ACS_VLINE);
    }
}

/* Highlighted bar across width cells of row y, so focus never relies on
 * colour alone. */
static void draw_focus_bar(int y, int x, int width, attr_t attrs)
{
    attr_on(attrs, NULL);
    for (int i = 0; i < width; i++)
    {
        mvaddch(y, x + i, ' ');
    }
    attr_off(attrs, NULL);
}

static void draw_entered_number(int y, int x, int w, int value, bool focused)
{
    attr_t attrs = text_style_attr(TEXT_STYLE_ENTERED);
    if (focused)
    {
        attrs |= A_REVERSE;
    }
    attr_on(attrs, NULL);
    draw_centered_number(y, x, w, value);
    attr_off(attrs, NULL);
}

static bool is_group_focused(const Board* board, int cell, GroupField field)
{
    return board->focus == BOARD_FOCUS_GROUP && board->group_cursor == cell &&
           board->group_field == field;
}

static char operation_symbol(GroupOperation operation)
{
    switch (operation)
    {
        case GROUP_OPERATION_ADD:
            return LAYOUT_ADD_SYMBOL;
        case GROUP_OPERATION_MULTIPLY:
            return LAYOUT_MULTIPLY_SYMBOL;
        case GROUP_OPERATION_NONE:
            break;
    }
    return ' ';
}

static void draw_group_field(const Board* board, int cell, GroupField field)
{
    int row                   = cell / TETONOR_GRID_SIZE;
    int col                   = cell % TETONOR_GRID_SIZE;
    const OperandGroup* group = &board->groups[cell];
    LayoutPoint origin        = layout_group_field_origin(row, col, field);
    int width                 = layout_group_field_width(field);
    bool focused              = is_group_focused(board, cell, field);
    attr_t attrs              = text_style_attr(TEXT_STYLE_ENTERED);
    int number = field == GROUP_FIELD_SMALLER ? group->smaller : group->greater;

    if (focused)
    {
        attrs |= A_REVERSE;
        draw_focus_bar(origin.y, origin.x, width, A_REVERSE);
    }
    attr_on(attrs, NULL);
    if (field == GROUP_FIELD_OPERATION)
    {
        mvaddch(origin.y, origin.x,
                (chtype)(unsigned char)operation_symbol(group->operation));
    }
    else if (number != TETONOR_EMPTY_ENTRY)
    {
        mvprintw(origin.y, origin.x, "%*d", width, number);
    }
    attr_off(attrs, NULL);
}

static void print_group_row(const Board* board, int cell)
{
    static const GroupField fields[] = {
        GROUP_FIELD_SMALLER, GROUP_FIELD_OPERATION, GROUP_FIELD_GREATER};
    int row           = cell / TETONOR_GRID_SIZE;
    int col           = cell % TETONOR_GRID_SIZE;
    LayoutPoint block = layout_sum_cell_origin(row, col);

    draw_vertical_rules(block.y + LAYOUT_CELL_GROUP_ROW, row, col, true);
    for (size_t i = 0; i < sizeof fields / sizeof fields[0]; i++)
    {
        draw_group_field(board, cell, fields[i]);
    }
}

static void print_block(const Board* board, int cell)
{
    int row           = cell / TETONOR_GRID_SIZE;
    int col           = cell % TETONOR_GRID_SIZE;
    LayoutPoint block = layout_sum_cell_origin(row, col);

    if (row == 0)
    {
        draw_rule(block.y, row, col, ACS_ULCORNER, NO_SEPARATOR, ACS_URCORNER);
    }
    else
    {
        draw_rule(block.y, row, col, ACS_LTEE, ACS_BTEE, ACS_RTEE);
    }
    draw_vertical_rules(block.y + LAYOUT_CELL_VALUE_ROW, row, col, false);
    draw_centered_number(block.y + LAYOUT_CELL_VALUE_ROW, block.x + 1,
                         BLOCK_INNER_W, board->puzzle->sum_matrix[row][col]);
    draw_rule(block.y + LAYOUT_CELL_SPLIT_ROW, row, col, ACS_LTEE, ACS_TTEE,
              ACS_RTEE);
    print_group_row(board, cell);
    if (row == TETONOR_GRID_SIZE - 1)
    {
        draw_rule(block.y + LAYOUT_CELL_PITCH_Y, row, col, ACS_LLCORNER,
                  ACS_BTEE, ACS_LRCORNER);
    }
}

static void print_sum_matrix(const Board* board)
{
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        print_block(board, cell);
    }
}

static void print_available_slot(const Board* board, int index)
{
    LayoutPoint origin    = layout_available_cell_origin(index);
    const BoardSlot* slot = &board->slots[index];
    int inner_w           = LAYOUT_MINI_W - 2;

    draw_box(origin.y, origin.x, LAYOUT_MINI_H, LAYOUT_MINI_W);

    if (!slot->editable)
    {
        draw_centered_number(origin.y + 1, origin.x, LAYOUT_MINI_W,
                             board->puzzle->available_array[index]);
        return;
    }
    bool focused = board->focus == BOARD_FOCUS_SLOT && index == board->cursor;
    if (focused)
    {
        draw_focus_bar(origin.y + 1, origin.x + 1, inner_w, A_REVERSE);
    }
    if (slot->value != TETONOR_EMPTY_ENTRY)
    {
        draw_entered_number(origin.y + 1, origin.x, LAYOUT_MINI_W, slot->value,
                            focused);
    }
}

static void print_available_array(const Board* board)
{
    LayoutPoint label = layout_available_label_origin();
    mvprintw(label.y, label.x, "Available:");

    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        print_available_slot(board, i);
    }
}

/* The player's way to submit the whole answer. Focus is shown by a
 * reverse-video bar, never by colour alone. */
static void print_confirm_button(const Board* board)
{
    LayoutPoint origin = layout_confirm_button_origin();
    bool focused       = board->focus == BOARD_FOCUS_SUBMIT;
    attr_t label_attrs = focused ? (A_REVERSE | A_BOLD) : A_NORMAL;

    draw_box(origin.y, origin.x, LAYOUT_BUTTON_H, LAYOUT_BUTTON_W);
    if (focused)
    {
        draw_focus_bar(origin.y + 1, origin.x + 1, LAYOUT_BUTTON_W - 2,
                       label_attrs);
    }
    int label_len = (int)strlen(LAYOUT_CONFIRM_LABEL);
    attr_on(label_attrs, NULL);
    mvprintw(origin.y + 1,
             origin.x + layout_centered_offset(LAYOUT_BUTTON_W, label_len),
             "%s", LAYOUT_CONFIRM_LABEL);
    attr_off(label_attrs, NULL);
}

static void print_result(const Board* board)
{
    if (board->result == BOARD_RESULT_PENDING)
    {
        return;
    }
    bool correct = board->result == BOARD_RESULT_CORRECT;
    attr_t attrs =
        text_style_attr(correct ? TEXT_STYLE_CORRECT : TEXT_STYLE_WRONG);
    LayoutPoint origin = layout_result_message_origin();
    attr_on(attrs, NULL);
    mvprintw(origin.y, origin.x, "%s",
             correct ? correct_message : wrong_message);
    attr_off(attrs, NULL);
}
