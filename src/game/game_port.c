#include <stdlib.h>

#include "game_port.h"
#include "number_generation.h"

GamePort* create_game_port(void)
{
    GamePort* game_port                  = malloc(sizeof(GamePort));
    game_port->difficulty                = 1;
    game_port->generate_random_numbers_f = &generate_random_numbers;
    game_port->free_number_generation_f  = &free_number_generation;
    return game_port;
}

void destroy_game_port(GamePort* game_port)
{
    free(game_port);
}
