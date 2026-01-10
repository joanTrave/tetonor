#include <ncurses.h>

#include "init_screen.h"

void init_screen(void)
{
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    refresh();
}
