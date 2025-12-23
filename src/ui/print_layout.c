#include <ncurses.h>

#include "print_layout.h"

void _print_sum_matrix(NumberGeneration*);
void _print_available_array(NumberGeneration*);

void print_layout(GamePort *game_port) {
    NumberGeneration *number_generation = game_port->generate_random_numbers_f(game_port->difficulty);
    _print_sum_matrix(number_generation);
    _print_available_array(number_generation);
    game_port->free_number_generation_f(number_generation);
}

void _print_sum_matrix(NumberGeneration *number_generation){
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            mvprintw(1 + i, 5 * j, "%d", number_generation->sum_matrix[i][j]);
        }
    }
}

void _print_available_array(NumberGeneration *number_generation)
{
    for (int i = 0; i < 16; i++) {
        mvprintw(7, 3 * i, "%d", number_generation->available_array[i]);
    }
}
