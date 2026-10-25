#ifndef TETONOR_PRINT_LAYOUT_H
#define TETONOR_PRINT_LAYOUT_H

#include <stdbool.h>
#include <stdint.h>

#include "pause_menu.h"
#include "victory_menu.h"
#include "../game/game_port.h"

/* Draws the puzzle, the player's entries, the cursor and the result.
 * undo_notice also shows the nothing-to-undo message; elapsed_seconds is the
 * game time shown by the timer. Does not own the data. */
void print_layout(const Board* board, bool undo_notice,
                  int32_t elapsed_seconds);

/* Draws the paused screen: the frozen timer and the pause menu or the prompt;
 * the board is hidden. */
void print_pause_screen(const PauseState* pause, int32_t elapsed_seconds);

/* Draws the victory screen: the summary and its two options; the board and the
 * timer are hidden. */
void print_victory_screen(const VictoryState* victory);

#endif
