#include <curses.h>

#include "screen.h"

void init_screen(void)
{
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    refresh();
}

void end_screen(void)
{
    endwin();
}
