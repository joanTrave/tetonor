#ifndef TETONOR_GAME_PORT_H
#define TETONOR_GAME_PORT_H

#include "number_generation.h"

typedef struct {
    int difficulty;

    /* Number generation related functions */
    NumberGeneration* (*generate_random_numbers_f)(int);
    void (*free_number_generation_f)(NumberGeneration*);
} GamePort;

GamePort *create_game_port(void);
void destroy_game_port(GamePort *game_port);

#endif
