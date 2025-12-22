#include <ncurses.h>

int main(void) {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    mvprintw(0, 0, "Tetonor - press any key to exit");
    refresh();

    getch();
    endwin();
    return 0;
}
