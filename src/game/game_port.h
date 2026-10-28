#ifndef TETONOR_GAME_PORT_H
#define TETONOR_GAME_PORT_H

#include <stdbool.h>
#include <time.h>

#include "board.h"
#include "daily_challenge.h"
#include "game_timer.h"
#include "number_generation.h"
#include "palette.h"
#include "saved_game.h"
#include "statistics.h"

#define TETONOR_DEFAULT_DIFFICULTY DIFFICULTY_MEDIUM

/* Which kind of game a port drives. NORMAL must stay 0. */
typedef enum
{
    GAME_MODE_NORMAL,
    GAME_MODE_DAILY_CHALLENGE
} GameMode;

typedef struct
{
    int difficulty; /* a Difficulty; fixed for one game, restart reuses it */
    GameMode mode;  /* NORMAL unless prepare_daily_challenge succeeded */
    CalendarDate daily_date; /* the challenge's UTC day; daily mode only */

    /* Number generation related functions */
    NumberGeneration* (*generate_random_numbers_f)(int difficulty);
    void (*free_number_generation_f)(NumberGeneration* number_generation);
    /* Released with free_number_generation_f. */
    NumberGeneration* (*generate_daily_challenge_f)(CalendarDate date);

    /* Board entry related functions */
    void (*init_board_f)(Board* board, const NumberGeneration* puzzle);
    BoardEvent (*apply_board_input_f)(Board* board, BoardInput input);
    bool (*undo_board_move_f)(Board* board);
    bool (*is_board_locked_f)(const Board* board);
    bool (*board_progress_would_be_lost_f)(const Board* board);
    int (*board_move_count_f)(const Board* board);

    /* Game timer related functions */
    void (*start_game_timer_f)(GameTimer* timer, time_t now);
    bool (*update_game_timer_f)(GameTimer* timer, BoardResult result,
                                time_t now);
    void (*pause_game_timer_f)(GameTimer* timer, time_t now);
    void (*resume_game_timer_f)(GameTimer* timer, time_t now);

    /* Saved game related functions */
    bool (*capture_saved_game_f)(const Board* board, int32_t elapsed_seconds,
                                 int difficulty, SavedGame* out);
    /* The returned puzzle is released with free_number_generation_f. */
    NumberGeneration* (*restore_saved_game_f)(const SavedGame* saved,
                                              Board* board, GameTimer* timer,
                                              time_t now);
} GamePort;

/* Returns NULL on allocation failure. */
GamePort* create_game_port(void);

/* Accepts NULL. */
void destroy_game_port(GamePort* game_port);

/* Turns the port into today's daily challenge: mode DAILY_CHALLENGE,
 * daily_date the UTC day of now, difficulty daily_challenge_difficulty of it.
 * Returns false, changing nothing, when the date cannot be computed. */
bool prepare_daily_challenge(GamePort* game_port, time_t now);

/* The port's next puzzle: generate_daily_challenge_f(daily_date) in daily
 * mode, generate_random_numbers_f(difficulty) otherwise. NULL on failure;
 * released with free_number_generation_f. */
NumberGeneration* generate_game_puzzle(const GamePort* game_port);

/* False for a daily challenge (never recorded in the statistics). */
bool game_port_records_statistics(const GamePort* game_port);

/* False for a daily challenge (never saved, never linked to the save). */
bool game_port_allows_saving(const GamePort* game_port);

/* Starts a brand new game: generates a new puzzle through generate_game_puzzle
 * (in daily mode, the same daily puzzle again), restarts
 * board over it and frees the old *puzzle through the port. Requires
 * board->puzzle == *puzzle. On generation failure returns false and leaves
 * *puzzle and *board untouched. Ownership of *puzzle stays with the caller. */
bool restart_game(const GamePort* game_port, Board* board,
                  NumberGeneration** puzzle);

#endif
