#ifndef TETONOR_GAME_TIMER_H
#define TETONOR_GAME_TIMER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "board.h"

#define TETONOR_TIMER_MAX_SECONDS                                              \
    359999                          /* 99:59:59: display, record, file cap */
#define TETONOR_ELAPSED_TEXT_SIZE 9 /* "99:59:59" plus the terminator */

/* Elapsed whole seconds of one game. A fixed-size value: nothing to release. */
typedef struct
{
    int32_t elapsed_seconds; /* 0..TETONOR_TIMER_MAX_SECONDS, never decreases */
    time_t last_tick;        /* now of the latest start or update */
    bool running;            /* false once the game is completed */
} GameTimer;

/* Starts (or restarts) at zero. */
void start_game_timer(GameTimer* timer, time_t now);

/* While running, adds the time since the previous call (ignoring clock steps
 * backwards, capped at TETONOR_TIMER_MAX_SECONDS); then, if result is
 * BOARD_RESULT_CORRECT, stops and returns true. Returns false otherwise, and
 * always once stopped, so a game completes at most once. */
bool update_game_timer(GameTimer* timer, BoardResult result, time_t now);

/* "MM:SS" below one hour, else "H:MM:SS"; seconds are clamped to
 * 0..TETONOR_TIMER_MAX_SECONDS. Returns false (out emptied) when size is too
 * small. */
bool format_elapsed_time(int32_t seconds, char* out, size_t size);

#endif
