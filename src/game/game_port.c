#include <stdlib.h>

#include "game_port.h"

GamePort* create_game_port(void)
{
    GamePort* game_port = malloc(sizeof *game_port);
    if (game_port == NULL)
    {
        return NULL;
    }
    game_port->difficulty                = TETONOR_DEFAULT_DIFFICULTY;
    game_port->generate_random_numbers_f = &generate_random_numbers;
    game_port->free_number_generation_f  = &free_number_generation;
    return game_port;
}

void destroy_game_port(GamePort* game_port)
{
    free(game_port);
}
