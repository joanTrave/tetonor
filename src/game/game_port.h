#ifndef TETONOR_GAME_PORT_H
#define TETONOR_GAME_PORT_H

#include <stdbool.h>

#include "board.h"
#include "number_generation.h"

#define TETONOR_DEFAULT_DIFFICULTY 1

typedef struct
{
    int difficulty;

    /* Number generation related functions */
    NumberGeneration* (*generate_random_numbers_f)(int difficulty);
    void (*free_number_generation_f)(NumberGeneration* number_generation);

    /* Board entry related functions */
    void (*init_board_f)(Board* board, const NumberGeneration* puzzle);
    BoardEvent (*apply_board_input_f)(Board* board, BoardInput input);
} GamePort;

/* Returns NULL on allocation failure. */
GamePort* create_game_port(void);

/* Accepts NULL. */
void destroy_game_port(GamePort* game_port);

/* Starts a brand new game: generates a new puzzle through the port, restarts
 * board over it and frees the old *puzzle through the port. Requires
 * board->puzzle == *puzzle. On generation failure returns false and leaves
 * *puzzle and *board untouched. Ownership of *puzzle stays with the caller. */
bool restart_game(const GamePort* game_port, Board* board,
                  NumberGeneration** puzzle);

#endif
