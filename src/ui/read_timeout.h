#ifndef TETONOR_UI_READ_TIMEOUT_H
#define TETONOR_UI_READ_TIMEOUT_H

#include <stdbool.h>
#include <time.h>

#define TETONOR_READ_TIMEOUT_MS 250
#define TETONOR_READ_CLOSED_STREAK 40
#define TETONOR_READ_CLOSED_WINDOW_SECONDS 2

typedef struct
{
    int consecutive_errors;
    time_t first_error_at; /* meaningful only when consecutive_errors > 0 */
} ReadTimeoutWatch;

void init_read_timeout_watch(ReadTimeoutWatch* watch);

/* A key arrived: forget earlier errors. */
void note_key_read(ReadTimeoutWatch* watch);

/* A read returned ERR at now. True when errors arrive far faster than the
 * timeout allows, which means the input is closed. */
bool note_read_error(ReadTimeoutWatch* watch, time_t now);

#endif
