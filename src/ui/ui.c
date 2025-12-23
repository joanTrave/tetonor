#include <ncurses.h>

#include "ui.h"
#include "init_screen.h"
#include "print_layout.h"

void ui_run(GamePort *game_port) {
    init_screen();
    print_layout(game_port);
    int ch = getch();
    while (ch != 27) { // 27 is the ASCII code for ESC
        ch = getch(); 
    }
    endwin();
}
