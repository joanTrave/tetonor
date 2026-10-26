#ifndef TETONOR_SAVED_GAME_H
#define TETONOR_SAVED_GAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "board.h"
#include "game_timer.h"
#include "number_generation.h"

/* Longest saved-game file, in bytes. */
#define TETONOR_SAVED_GAME_TEXT_MAX 1024

/* Everything needed to continue one game. A fixed-size value: nothing to
 * release. */
typedef struct
{
    int difficulty;          /* a Difficulty */
    int32_t elapsed_seconds; /* 0..TETONOR_TIMER_MAX_SECONDS */
    int move_count;          /* see board_move_count */
    NumberGeneration puzzle; /* full copy, including the generator's solution */
    BoardSnapshot entries;   /* slot values, group boxes and result */
} SavedGame;

/* Copies the game into *out. Returns false, leaving *out untouched, when the
 * board has no puzzle, is locked, or the result breaks is_saved_game_valid. */
bool capture_saved_game(const Board* board, int32_t elapsed_seconds,
                        int difficulty, SavedGame* out);

/* Ranges and consistency of every field: the difficulty, the time and the move
 * count; a solvable puzzle of the right shape for the difficulty; entries that
 * fit the puzzle; a result the entries justify; and no more filled boxes than
 * moves. Checks every range before it uses a value as an index. */
bool is_saved_game_valid(const SavedGame* saved);

/* Writes the versioned text format, terminated by '\0'. Does not validate.
 * Returns false if the text does not fit size. */
bool serialize_saved_game(const SavedGame* saved, char* out, size_t size);

/* Parses exactly length bytes, then validates. Returns false, leaving *saved
 * untouched, on any malformed, wrong-version, out-of-range or inconsistent
 * input. */
bool parse_saved_game(const char* text, size_t length, SavedGame* saved);

/* Validates saved, then copies its puzzle to the heap, restores board over it
 * (restore_board) and timer (restore_game_timer at now). Returns the puzzle,
 * owned by the caller and released with free_number_generation; NULL, leaving
 * board and timer untouched, when saved is invalid or allocation fails. */
NumberGeneration* restore_saved_game(const SavedGame* saved, Board* board,
                                     GameTimer* timer, time_t now);

#endif
