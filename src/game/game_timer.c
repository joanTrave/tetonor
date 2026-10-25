#include <stdio.h>

#include "game_timer.h"

#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR 3600

static int32_t clamp_seconds(int32_t seconds)
{
    if (seconds < 0)
    {
        return 0;
    }
    if (seconds > TETONOR_TIMER_MAX_SECONDS)
    {
        return TETONOR_TIMER_MAX_SECONDS;
    }
    return seconds;
}

/* Clamps in double first, so a huge clock jump never overflows the cast. */
static int32_t clamp_delta(double delta)
{
    if (delta > TETONOR_TIMER_MAX_SECONDS)
    {
        return TETONOR_TIMER_MAX_SECONDS;
    }
    return (int32_t)delta;
}

/* Adds the time since the previous tick; a clock step backwards adds nothing.
 */
static void accrue_elapsed(GameTimer* timer, time_t now)
{
    double delta     = difftime(now, timer->last_tick);
    timer->last_tick = now;
    if (delta > 0)
    {
        timer->elapsed_seconds =
            clamp_seconds(timer->elapsed_seconds + clamp_delta(delta));
    }
}

void start_game_timer(GameTimer* timer, time_t now)
{
    timer->elapsed_seconds = 0;
    timer->last_tick       = now;
    timer->running         = true;
    timer->paused          = false;
}

bool update_game_timer(GameTimer* timer, BoardResult result, time_t now)
{
    if (!timer->running || timer->paused)
    {
        return false;
    }
    accrue_elapsed(timer, now);
    if (result != BOARD_RESULT_CORRECT)
    {
        return false;
    }
    timer->running = false;
    return true;
}

void pause_game_timer(GameTimer* timer, time_t now)
{
    if (timer->paused)
    {
        return;
    }
    if (timer->running)
    {
        accrue_elapsed(timer, now);
    }
    timer->paused = true;
}

void resume_game_timer(GameTimer* timer, time_t now)
{
    if (!timer->paused)
    {
        return;
    }
    timer->last_tick = now;
    timer->paused    = false;
}

bool format_elapsed_time(int32_t seconds, char* out, size_t size)
{
    if (size == 0)
    {
        return false;
    }
    int32_t total = clamp_seconds(seconds);
    int written;
    if (total < SECONDS_PER_HOUR)
    {
        written =
            snprintf(out, size, "%02d:%02d", (int)(total / SECONDS_PER_MINUTE),
                     (int)(total % SECONDS_PER_MINUTE));
    }
    else
    {
        written =
            snprintf(out, size, "%d:%02d:%02d", (int)(total / SECONDS_PER_HOUR),
                     (int)(total % SECONDS_PER_HOUR / SECONDS_PER_MINUTE),
                     (int)(total % SECONDS_PER_MINUTE));
    }
    if (written < 0 || (size_t)written >= size)
    {
        out[0] = '\0';
        return false;
    }
    return true;
}
