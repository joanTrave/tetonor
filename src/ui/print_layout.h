#ifndef TETONOR_PRINT_LAYOUT_H
#define TETONOR_PRINT_LAYOUT_H

#include <stdbool.h>

#include "../game/game_port.h"

/* Draws the puzzle, the player's entries, the cursor and the result.
 * undo_notice also shows the nothing-to-undo message. Does not own the data. */
void print_layout(const Board* board, bool undo_notice);

#endif
