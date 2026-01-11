#include <ncurses.h>
#include <stdio.h>
#include "menu.h"
#include "ui.h"
#include "init_screen.h"
#include "../game/game_port.h"

#define WIN_H 7
#define WIN_W 50

WINDOW* _init_window(void);
void _draw_main_menu(WINDOW* win);
MenuResult _select_menu_option(WINDOW* win);

void ui_show_start_menu(void)
{
    init_screen();
    WINDOW* win         = _init_window();
    GamePort* game_port = create_game_port();
    _draw_main_menu(win);
    MenuResult menu_result = _select_menu_option(win);
    if (menu_result == MENU_EXIT)
    {
        endwin();
        return;
    }
    clear();
    refresh();
    ui_run(game_port);
    destroy_game_port(game_port);
}

WINDOW* _init_window(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int start_y = (rows - WIN_H) / 2;
    int start_x = (cols - WIN_W) / 2;

    return newwin(WIN_H, WIN_W, start_y, start_x);
}

void _draw_main_menu(WINDOW* win)
{
    keypad(win, TRUE);

    box(win, 0, 0);

    mvwprintw(win, 1, (WIN_W - 17) / 2, "Welcome to Tetonor");
    mvwprintw(win, 3, 2, "Press SPACE to start game");
    mvwprintw(win, 4, 2, "Press ESC to exit");

    wrefresh(win);
}

MenuResult _select_menu_option(WINDOW* win)
{
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
