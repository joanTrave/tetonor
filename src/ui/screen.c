#include <curses.h>

#include "screen.h"
#include "style.h"

void init_screen(void)
{
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    init_styles();
    refresh();
}

void end_screen(void)
{
    endwin();
}
