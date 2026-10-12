#include <curses.h>
#include <stdio.h>
#include <string.h>

#include "print_layout.h"
#include "layout.h"

#define NUMBER_BUFFER_SIZE 8

static void print_sum_matrix(const NumberGeneration* number_generation);
static void print_available_array(const NumberGeneration* number_generation);

void print_layout(const NumberGeneration* number_generation)
{
    print_sum_matrix(number_generation);
    print_available_array(number_generation);
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

static void print_available_array(const NumberGeneration* number_generation)
{
    LayoutPoint label = layout_available_label_origin();
    mvprintw(label.y, label.x, "Available:");

    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        LayoutPoint origin = layout_available_cell_origin(i);

        draw_box(origin.y, origin.x, LAYOUT_MINI_H, LAYOUT_MINI_W);

        if (number_generation->available_array[i] != TETONOR_HIDDEN_NUMBER)
        {
            draw_centered_number(origin.y + 1, origin.x, LAYOUT_MINI_W,
                                 number_generation->available_array[i]);
        }
    }
}
