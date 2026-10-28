#ifndef TETONOR_PRINT_LAYOUT_H
#define TETONOR_PRINT_LAYOUT_H

#include <stdbool.h>
#include <stdint.h>

#include "pause_menu.h"
#include "theme_menu.h"
#include "victory_menu.h"
#include "../game/game_port.h"

/* Draws the puzzle, the player's entries, the cursor and the result.
 * undo_notice also shows the nothing-to-undo message; hint, unless NULL, is the
 * reply to show under the timer and, for a suggestion, whose target is framed
 * by LAYOUT_HINT_MARKER; uncounted also shows that the game will not count for
 * statistics; elapsed_seconds is the game time shown by the timer. Does not own
 * the data. */
void print_layout(const Board* board, bool undo_notice, const BoardHint* hint,
                  bool uncounted, int32_t elapsed_seconds);

/* Draws the paused screen: the frozen timer and the pause menu or the prompt;
 * the board is hidden. can_save chooses the save hint: false shows that the
 * game cannot be saved. */
void print_pause_screen(const PauseState* pause, int32_t elapsed_seconds,
                        bool can_save);

/* Draws the victory screen: the summary and its two options; the board and the
 * timer are hidden. */
void print_victory_screen(const VictoryState* victory);

/* Draws the theme screen: one option per theme, the active one marked; the
 * board and the timer are hidden. */
void print_theme_screen(const ThemeMenu* menu);

/* Draws the "DAILY CHALLENGE YYYY-MM-DD" badge on row 0 in reversed bold
 * video, so its meaning does not depend on colour. Draws nothing when the
 * terminal is too small (row 0 then holds the enlarge message). */
void print_daily_challenge_badge(CalendarDate date);

#endif
