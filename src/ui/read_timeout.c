#include "read_timeout.h"

void init_read_timeout_watch(ReadTimeoutWatch* watch)
{
    watch->consecutive_errors = 0;
    watch->first_error_at     = 0;
}

void note_key_read(ReadTimeoutWatch* watch)
{
    watch->consecutive_errors = 0;
}

bool note_read_error(ReadTimeoutWatch* watch, time_t now)
{
    if (watch->consecutive_errors == 0)
    {
        watch->first_error_at = now;
    }
    watch->consecutive_errors++;
    if (watch->consecutive_errors < TETONOR_READ_CLOSED_STREAK)
    {
        return false;
    }
    double span               = difftime(now, watch->first_error_at);
    watch->consecutive_errors = 0;
    return span >= 0 && span <= TETONOR_READ_CLOSED_WINDOW_SECONDS;
}
