#include <ncurses.h>

#include "init_screen.h"

void init_screen(void) {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    mvprintw(0, 0, "Tetonor - press ESC to exit");
    refresh();
}
