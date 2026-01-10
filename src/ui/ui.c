#include <ncurses.h>

#include "ui.h"
#include "init_screen.h"
#include "menu.h"
#include "print_layout.h"

void ui_run(GamePort* game_port)
{
    init_screen();
    MenuResult menu_result = ui_show_start_menu();
    if (menu_result == MENU_EXIT)
    {
        endwin();
        return;
    }
    clear();
    refresh();
    print_layout(game_port);
    int ch = getch();
    while (ch != 27)
    { // 27 is the ASCII code for ESC
        ch = getch();
    }
    endwin();
}
