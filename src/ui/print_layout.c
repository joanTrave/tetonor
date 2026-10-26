#include <curses.h>
#include <stdio.h>
#include <string.h>

#include "print_layout.h"
#include "layout.h"
#include "palette_style.h"
#include "style.h"

#define NUMBER_BUFFER_SIZE 8
#define LAYOUT_ADD_SYMBOL '+'
#define LAYOUT_MULTIPLY_SYMBOL 'x'
#define BLOCK_INNER_W (LAYOUT_CELL_W - 2)
#define NO_SEPARATOR ((chtype)0)

static void print_sum_matrix(const Board* board);
static void print_available_array(const Board* board);
static void print_button(LayoutPoint origin, const char* label, bool focused);
static void print_confirm_button(const Board* board);
static void print_restart_button(const Board* board);
static void print_result(const Board* board);
static void print_undo_notice(void);
static void print_timer(int32_t elapsed_seconds);
static void print_controls_help(void);

void print_layout(const Board* board, bool undo_notice, int32_t elapsed_seconds)
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
    print_restart_button(board);
    print_timer(elapsed_seconds);
    print_result(board);
    if (undo_notice)
    {
        print_undo_notice();
    }
    print_controls_help();
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

static void draw_styled_number(int y, int x, int w, int value, attr_t attrs)
{
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
    attr_t attrs              = palette_role_attr(PALETTE_ROLE_ENTERED);
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

    bool focused = slot->editable && board->focus == BOARD_FOCUS_SLOT &&
                   index == board->cursor;
    if (focused)
    {
        draw_focus_bar(origin.y + 1, origin.x + 1, inner_w, A_REVERSE);
    }
    int value =
        slot->editable ? slot->value : board->puzzle->available_array[index];
    if (value == TETONOR_EMPTY_ENTRY)
    {
        return;
    }
    PaletteRole role = PALETTE_ROLE_ENTERED;
    attr_t attrs     = A_NORMAL;
    if (strip_slot_role(slot->editable, board->slot_usages[index], &role))
    {
        attrs = palette_role_attr(role);
    }
    if (focused)
    {
        attrs |= A_REVERSE;
    }
    draw_styled_number(origin.y + 1, origin.x, LAYOUT_MINI_W, value, attrs);
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
    print_button(layout_confirm_button_origin(), LAYOUT_CONFIRM_LABEL,
                 board->focus == BOARD_FOCUS_SUBMIT);
}

/* The player's way to start a brand new game, drawn like CONFIRM. */
static void print_restart_button(const Board* board)
{
    print_button(layout_restart_button_origin(), LAYOUT_RESTART_LABEL,
                 board->focus == BOARD_FOCUS_RESTART);
}

static void print_button(LayoutPoint origin, const char* label, bool focused)
{
    attr_t label_attrs = focused ? (A_REVERSE | A_BOLD) : A_NORMAL;

    draw_box(origin.y, origin.x, LAYOUT_BUTTON_H, LAYOUT_BUTTON_W);
    if (focused)
    {
        draw_focus_bar(origin.y + 1, origin.x + 1, LAYOUT_BUTTON_W - 2,
                       label_attrs);
    }
    int label_len = (int)strlen(label);
    attr_on(label_attrs, NULL);
    mvprintw(origin.y + 1,
             origin.x + layout_centered_offset(LAYOUT_BUTTON_W, label_len),
             "%s", label);
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
        palette_role_attr(correct ? PALETTE_ROLE_CORRECT : PALETTE_ROLE_WRONG);
    LayoutPoint origin = layout_result_message_origin();
    attr_on(attrs, NULL);
    mvprintw(origin.y, origin.x, "%s",
             correct ? LAYOUT_CORRECT_MESSAGE : LAYOUT_WRONG_MESSAGE);
    attr_off(attrs, NULL);
}

static void print_undo_notice(void)
{
    LayoutPoint origin = layout_undo_notice_origin();
    attr_on(A_BOLD, NULL);
    mvprintw(origin.y, origin.x, "%s", LAYOUT_UNDO_EMPTY_MESSAGE);
    attr_off(A_BOLD, NULL);
}

static void print_timer(int32_t elapsed_seconds)
{
    char text[TETONOR_ELAPSED_TEXT_SIZE];
    format_elapsed_time(elapsed_seconds, text, sizeof text);
    LayoutPoint origin = layout_timer_origin();
    mvprintw(origin.y, origin.x, "%s%s", LAYOUT_TIMER_LABEL, text);
}

static void print_controls_help(void)
{
    for (int i = 0; i < LAYOUT_HELP_LINE_COUNT; i++)
    {
        LayoutPoint origin = layout_help_line_origin(i);
        mvprintw(origin.y, origin.x, "%s", layout_help_line(i));
    }
}

/* "> " and reversed video mark the highlighted option, as in menu.c. */
static void print_panel_option(LayoutPoint panel, int row, const char* label,
                               bool selected)
{
    if (selected)
    {
        attr_on(A_REVERSE, NULL);
    }
    mvprintw(panel.y + row, panel.x + LAYOUT_MENU_TEXT_COL, "%s%s",
             selected ? "> " : "  ", label);
    if (selected)
    {
        attr_off(A_REVERSE, NULL);
    }
}

static void print_panel_text(LayoutPoint panel, int row, const char* text)
{
    mvprintw(panel.y + row, panel.x + LAYOUT_MENU_TEXT_COL, "%s", text);
}

static void print_panel_title(LayoutPoint panel, int width, int row,
                              const char* title)
{
    mvprintw(panel.y + row,
             panel.x + layout_centered_offset(width, (int)strlen(title)), "%s",
             title);
}

static void print_pause_panel(const PauseState* pause, int rows, int cols)
{
    LayoutPoint panel = layout_centered_window_origin(
        rows, cols, LAYOUT_PAUSE_MENU_H, LAYOUT_PAUSE_MENU_W);
    draw_box(panel.y, panel.x, LAYOUT_PAUSE_MENU_H, LAYOUT_PAUSE_MENU_W);
    print_panel_title(panel, LAYOUT_PAUSE_MENU_W, LAYOUT_PAUSE_TITLE_ROW,
                      LAYOUT_PAUSE_TITLE);
    for (int option = 0; option < PAUSE_OPTION_COUNT; option++)
    {
        print_panel_option(panel, layout_pause_option_row(option),
                           pause_option_label(option),
                           option == (int)pause->selected);
    }
    print_panel_text(panel, LAYOUT_PAUSE_CHOOSE_ROW, LAYOUT_PAUSE_CHOOSE_HINT);
    print_panel_text(panel, LAYOUT_PAUSE_BACK_ROW, LAYOUT_PAUSE_BACK_HINT);
}

static void print_prompt_panel(const PauseState* pause, int rows, int cols)
{
    LayoutPoint panel = layout_centered_window_origin(
        rows, cols, LAYOUT_PROMPT_H, LAYOUT_PROMPT_W);
    draw_box(panel.y, panel.x, LAYOUT_PROMPT_H, LAYOUT_PROMPT_W);
    print_panel_title(panel, LAYOUT_PROMPT_W, LAYOUT_PROMPT_TITLE_ROW,
                      prompt_question(pause->pending));
    print_panel_text(panel, LAYOUT_PROMPT_WARNING_ROW, LAYOUT_PROMPT_WARNING);
    for (int option = 0; option < PROMPT_OPTION_COUNT; option++)
    {
        print_panel_option(panel, layout_prompt_option_row(option),
                           prompt_option_label(option),
                           option == (int)pause->answer);
    }
    print_panel_text(panel, LAYOUT_PROMPT_CHOOSE_ROW,
                     LAYOUT_PROMPT_CHOOSE_HINT);
    print_panel_text(panel, LAYOUT_PROMPT_BACK_ROW, LAYOUT_PROMPT_BACK_HINT);
}

void print_victory_screen(const VictoryState* victory)
{
    int rows = 0;
    int cols = 0;
    getmaxyx(stdscr, rows, cols);
    if (!layout_fits(rows, cols))
    {
        mvprintw(0, 0, LAYOUT_ENLARGE_FORMAT, LAYOUT_MIN_COLS, LAYOUT_MIN_ROWS);
        return;
    }
    LayoutPoint panel = layout_centered_window_origin(
        rows, cols, LAYOUT_VICTORY_H, LAYOUT_VICTORY_W);
    draw_box(panel.y, panel.x, LAYOUT_VICTORY_H, LAYOUT_VICTORY_W);
    print_panel_title(panel, LAYOUT_VICTORY_W, LAYOUT_VICTORY_TITLE_ROW,
                      LAYOUT_VICTORY_TITLE);
    for (int line = 0; line < VICTORY_SUMMARY_LINE_COUNT; line++)
    {
        print_panel_text(panel, layout_victory_summary_row(line),
                         victory->summary.lines[line]);
    }
    for (int option = 0; option < VICTORY_OPTION_COUNT; option++)
    {
        print_panel_option(panel, layout_victory_option_row(option),
                           victory_option_label(option),
                           option == (int)victory->selected);
    }
    print_panel_text(panel, LAYOUT_VICTORY_CHOOSE_ROW,
                     LAYOUT_VICTORY_CHOOSE_HINT);
    print_panel_text(panel, LAYOUT_VICTORY_SHORTCUT_ROW,
                     LAYOUT_VICTORY_SHORTCUT_HINT);
}

void print_pause_screen(const PauseState* pause, int32_t elapsed_seconds)
{
    int rows = 0;
    int cols = 0;
    getmaxyx(stdscr, rows, cols);
    if (!layout_fits(rows, cols))
    {
        mvprintw(0, 0, LAYOUT_ENLARGE_FORMAT, LAYOUT_MIN_COLS, LAYOUT_MIN_ROWS);
        return;
    }
    print_timer(elapsed_seconds);
    switch (pause->screen)
    {
        case PAUSE_SCREEN_MENU:
            print_pause_panel(pause, rows, cols);
            break;
        case PAUSE_SCREEN_PROMPT:
            print_prompt_panel(pause, rows, cols);
            break;
        case PAUSE_SCREEN_THEME:
        case PAUSE_SCREEN_PLAYING:
            break;
    }
}

void print_theme_screen(const ThemeMenu* menu)
{
    int rows = 0;
    int cols = 0;
    getmaxyx(stdscr, rows, cols);
    if (!layout_fits(rows, cols))
    {
        mvprintw(0, 0, LAYOUT_ENLARGE_FORMAT, LAYOUT_MIN_COLS, LAYOUT_MIN_ROWS);
        return;
    }
    LayoutPoint panel = layout_centered_window_origin(
        rows, cols, LAYOUT_THEME_MENU_H, LAYOUT_THEME_MENU_W);
    draw_box(panel.y, panel.x, LAYOUT_THEME_MENU_H, LAYOUT_THEME_MENU_W);
    print_panel_title(panel, LAYOUT_THEME_MENU_W, LAYOUT_THEME_TITLE_ROW,
                      LAYOUT_THEME_TITLE);
    for (int theme = 0; theme < PALETTE_THEME_COUNT; theme++)
    {
        char text[LAYOUT_THEME_MENU_W];
        snprintf(text, sizeof text, "%s%s", theme_menu_label(theme),
                 theme_menu_marker(menu, theme));
        print_panel_option(panel, layout_theme_option_row(theme), text,
                           theme == (int)menu->selected);
    }
    print_panel_text(panel, LAYOUT_THEME_CHOOSE_ROW, LAYOUT_THEME_CHOOSE_HINT);
    print_panel_text(panel, LAYOUT_THEME_BACK_ROW, LAYOUT_THEME_BACK_HINT);
}
