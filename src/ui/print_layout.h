#ifndef TETONOR_PRINT_LAYOUT_H
#define TETONOR_PRINT_LAYOUT_H

#include "../game/game_port.h"

/* Draws the puzzle, the player's entries, the cursor and the result. Does not
 * own the data. */
void print_layout(const Board* board);

#endif
