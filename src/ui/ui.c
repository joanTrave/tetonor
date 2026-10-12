#include <curses.h>
#include <stdlib.h>

#include "ui.h"
#include "keys.h"
#include "print_layout.h"

int ui_run(const GamePort* game_port)
{
    NumberGeneration* number_generation =
        game_port->generate_random_numbers_f(game_port->difficulty);
    if (number_generation == NULL)
    {
        return EXIT_FAILURE;
    }

    print_layout(number_generation);
    refresh();

    int key = getch();
    while (key != TETONOR_KEY_ESCAPE && key != ERR)
    {
        key = getch();
    }

    game_port->free_number_generation_f(number_generation);
    return EXIT_SUCCESS;
}
