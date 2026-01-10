#include <ncurses.h>
#include "menu.h"

MenuResult ui_show_start_menu(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int win_h = 7;
    int win_w = 50;

    int start_y = (rows - win_h) / 2;
    int start_x = (cols - win_w) / 2;

    WINDOW *win = newwin(win_h, win_w, start_y, start_x);
    keypad(win, TRUE);

    box(win, 0, 0);

    mvwprintw(win, 1, (win_w - 17) / 2, "Welcome to Tetonor");
    mvwprintw(win, 3, 2, "Press SPACE to start game");
    mvwprintw(win, 4, 2, "Press ESC to exit");

    wrefresh(win);

    int ch;
    while (1)
    {
        ch = wgetch(win);

        if (ch == ' ')
        {
            delwin(win);
            return MENU_START;
        }

        if (ch == 27) // ESC
        {
            delwin(win);
            return MENU_EXIT;
        }
    }
}
