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
    bool paused; /* true between pause_game_timer and resume_game_timer */
} GameTimer;

/* Starts (or restarts) at zero, running and not paused. */
void start_game_timer(GameTimer* timer, time_t now);

/* Starts at now (running, not paused) with elapsed_seconds already counted,
 * clamped to 0..TETONOR_TIMER_MAX_SECONDS. */
void restore_game_timer(GameTimer* timer, int32_t elapsed_seconds, time_t now);

/* While running, adds the time since the previous call (ignoring clock steps
 * backwards, capped at TETONOR_TIMER_MAX_SECONDS); then, if result is
 * BOARD_RESULT_CORRECT, stops and returns true. Returns false otherwise, and
 * always once stopped, so a game completes at most once. Does nothing and
 * returns false while paused. */
bool update_game_timer(GameTimer* timer, BoardResult result, time_t now);

/* Counts the time up to now (like update_game_timer, without completing) and
 * stops counting until resume_game_timer. No effect when already paused; a
 * completed timer stays stopped. */
void pause_game_timer(GameTimer* timer, time_t now);

/* Counting continues from now; the time since pause_game_timer is not added.
 * No effect when not paused. */
void resume_game_timer(GameTimer* timer, time_t now);

/* "MM:SS" below one hour, else "H:MM:SS"; seconds are clamped to
 * 0..TETONOR_TIMER_MAX_SECONDS. Returns false (out emptied) when size is too
 * small. */
bool format_elapsed_time(int32_t seconds, char* out, size_t size);

#endif
