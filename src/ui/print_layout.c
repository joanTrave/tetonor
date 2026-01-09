#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#include "print_layout.h"

#define GRID_SIZE 4
#define CELL_W 6
#define CELL_H 3
#define AVAIL_COUNT 16
#define MINI_W 5
#define MINI_H 3

static void _print_sum_matrix(NumberGeneration*);
static void _print_available_array(NumberGeneration*);

void print_layout(GamePort* game_port)
{
    NumberGeneration* number_generation =
        game_port->generate_random_numbers_f(game_port->difficulty);
    _print_sum_matrix(number_generation);
    _print_available_array(number_generation);
    game_port->free_number_generation_f(number_generation);
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
    if (value < 0)
        return;

    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);

    int len = (int)strlen(buf);
    int px = x + (w - len) / 2;

    mvprintw(y, px, "%s", buf);
}


static void _print_sum_matrix(NumberGeneration* ng)
{
    int start_y = 2;
    int start_x = 4;

    for (int r = 0; r < GRID_SIZE; r++)
    {
        for (int c = 0; c < GRID_SIZE; c++)
        {
            int y = start_y + r * CELL_H;
            int x = start_x + c * CELL_W;

            draw_box(y, x, CELL_H, CELL_W);
            draw_centered_number(y + 1, x, CELL_W, ng->sum_matrix[r][c]);
        }
    }
}


static void _print_available_array(NumberGeneration* ng)
{
    int start_y = 2 + GRID_SIZE * CELL_H + 2;
    int start_x = 4;

    mvprintw(start_y - 1, start_x, "Available:");

    for (int i = 0; i < AVAIL_COUNT; i++)
    {
        int x = start_x + i * MINI_W;
        int y = start_y;

        draw_box(y, x, MINI_H, MINI_W);

        if (ng->available_array[i] != -1)
        {
            draw_centered_number(y + 1, x, MINI_W, ng->available_array[i]);
        }
    }
}

