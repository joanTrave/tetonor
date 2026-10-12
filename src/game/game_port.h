#ifndef TETONOR_GAME_PORT_H
#define TETONOR_GAME_PORT_H

#include "number_generation.h"

#define TETONOR_DEFAULT_DIFFICULTY 1

typedef struct
{
    int difficulty;

    /* Number generation related functions */
    NumberGeneration* (*generate_random_numbers_f)(int difficulty);
    void (*free_number_generation_f)(NumberGeneration* number_generation);
} GamePort;

/* Returns NULL on allocation failure. */
GamePort* create_game_port(void);

/* Accepts NULL. */
void destroy_game_port(GamePort* game_port);

#endif
