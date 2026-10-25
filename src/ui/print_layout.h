#ifndef TETONOR_PRINT_LAYOUT_H
#define TETONOR_PRINT_LAYOUT_H

#include <stdbool.h>
#include <stdint.h>

#include "../game/game_port.h"

/* Draws the puzzle, the player's entries, the cursor and the result.
 * undo_notice also shows the nothing-to-undo message; elapsed_seconds is the
 * game time shown by the timer. Does not own the data. */
void print_layout(const Board* board, bool undo_notice,
                  int32_t elapsed_seconds);

#endif
