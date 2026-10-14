#include <curses.h>
#include <stdio.h>
#include <string.h>

#include "print_layout.h"
#include "layout.h"
#include "style.h"

#define NUMBER_BUFFER_SIZE 8

static const char correct_message[] = "CORRECT ANSWER";
static const char wrong_message[]   = "WRONG ANSWER";

static void print_sum_matrix(const NumberGeneration* number_generation);
static void print_available_array(const Board* board);
static void print_confirm_button(const Board* board);
static void print_result(const Board* board);

void print_layout(const Board* board)
{
    print_sum_matrix(board->puzzle);
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

static void print_sum_matrix(const NumberGeneration* number_generation)
{
    for (int r = 0; r < TETONOR_GRID_SIZE; r++)
    {
        for (int c = 0; c < TETONOR_GRID_SIZE; c++)
        {
            LayoutPoint origin = layout_sum_cell_origin(r, c);

            draw_box(origin.y, origin.x, LAYOUT_CELL_H, LAYOUT_CELL_W);
            draw_centered_number(origin.y + 1, origin.x, LAYOUT_CELL_W,
                                 number_generation->sum_matrix[r][c]);
        }
    }
}

/* Highlighted bar across the inner cells of a box w columns wide, so focus
 * never relies on colour alone. */
static void draw_focus_bar(LayoutPoint origin, int w, attr_t focus)
{
    attr_on(focus, NULL);
    for (int i = 1; i < w - 1; i++)
    {
        mvaddch(origin.y + 1, origin.x + i, ' ');
    }
    attr_off(focus, NULL);
}

static void draw_entered_number(LayoutPoint origin, const BoardSlot* slot,
                                bool focused)
{
    attr_t attrs = text_style_attr(TEXT_STYLE_ENTERED);
    if (focused)
    {
        attrs |= A_REVERSE;
    }
    attr_on(attrs, NULL);
    draw_centered_number(origin.y + 1, origin.x, LAYOUT_MINI_W, slot->value);
    attr_off(attrs, NULL);
}

static void print_available_slot(const Board* board, int index)
{
    LayoutPoint origin    = layout_available_cell_origin(index);
    const BoardSlot* slot = &board->slots[index];

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
        draw_focus_bar(origin, LAYOUT_MINI_W, A_REVERSE);
    }
    if (slot->value != TETONOR_EMPTY_ENTRY)
    {
        draw_entered_number(origin, slot, focused);
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
        draw_focus_bar(origin, LAYOUT_BUTTON_W, label_attrs);
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
