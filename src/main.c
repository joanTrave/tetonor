#include <ncurses.h>
#include <stdlib.h>
#include <time.h>

#include "ui/menu.h"

void init_random_generator(void);

int main(void)
{
    init_random_generator();
    ui_show_start_menu();
    return 0;
}

void init_random_generator(void)
{
    srand((unsigned)time(NULL));
}
