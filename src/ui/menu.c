#include <curses.h>
#include <stdlib.h>
#include <string.h>

#include "menu.h"
#include "keys.h"
#include "layout.h"
#include "ui.h"
#include "../game/game_port.h"

static const char menu_title[]      = "Welcome to Tetonor";
static const char menu_start_hint[] = "Press SPACE or ENTER to start game";
static const char menu_exit_hint[]  = "Press ESC to exit";

static WINDOW* create_menu_window(void);
static void draw_main_menu(WINDOW* win);
static MenuResult read_menu_choice(WINDOW* win);

int ui_show_start_menu(void)
{
    WINDOW* win = create_menu_window();
    if (win == NULL)
    {
        return EXIT_FAILURE;
    }
    draw_main_menu(win);
    MenuResult menu_result = read_menu_choice(win);
    delwin(win);
    if (menu_result == MENU_EXIT)
    {
        return EXIT_SUCCESS;
    }

    clear();
    refresh();

    GamePort* game_port = create_game_port();
    if (game_port == NULL)
    {
        return EXIT_FAILURE;
    }
    int status = ui_run(game_port);
    destroy_game_port(game_port);
    return status;
}

static WINDOW* create_menu_window(void)
{
    int rows;
    int cols;
    getmaxyx(stdscr, rows, cols);

    LayoutPoint origin =
        layout_centered_window_origin(rows, cols, LAYOUT_MENU_H, LAYOUT_MENU_W);
    return newwin(LAYOUT_MENU_H, LAYOUT_MENU_W, origin.y, origin.x);
}

static void draw_main_menu(WINDOW* win)
{
    keypad(win, TRUE);

    box(win, 0, 0);

    int title_col =
        layout_centered_offset(LAYOUT_MENU_W, (int)strlen(menu_title));
    mvwprintw(win, LAYOUT_MENU_TITLE_ROW, title_col, "%s", menu_title);
    mvwprintw(win, LAYOUT_MENU_START_ROW, LAYOUT_MENU_TEXT_COL, "%s",
              menu_start_hint);
    mvwprintw(win, LAYOUT_MENU_EXIT_ROW, LAYOUT_MENU_TEXT_COL, "%s",
              menu_exit_hint);

    wrefresh(win);
}

static MenuResult read_menu_choice(WINDOW* win)
{
    MenuResult result = MENU_NONE;
    while (result == MENU_NONE)
    {
        int key = wgetch(win);
        if (key == ERR)
        {
            return MENU_EXIT; /* input closed: leave instead of spinning */
        }
        result = menu_result_from_key(key, KEY_ENTER);
    }
    return result;
}
