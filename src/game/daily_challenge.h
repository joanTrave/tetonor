#ifndef TETONOR_DAILY_CHALLENGE_H
#define TETONOR_DAILY_CHALLENGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "number_generation.h"

#define TETONOR_DATE_TEXT_SIZE 11 /* "YYYY-MM-DD" plus NUL */

/* A calendar day. month is 1..12, day is 1..31. */
typedef struct
{
    int year;
    int month;
    int day;
} CalendarDate;

/* The UTC calendar day containing now (gmtime). Returns false, leaving *out
 * untouched, when now is (time_t)-1 or cannot be converted. */
bool utc_calendar_date(time_t now, CalendarDate* out);

/* year * 10000 + month * 100 + day, e.g. 2026-10-27 gives 20261027. Exact for
 * years 0..9999. */
uint32_t daily_challenge_seed(CalendarDate date);

/* A Difficulty: day % 3, so 0 Easy, 1 Medium, 2 Hard. A negative day (which
 * utc_calendar_date never yields) gives a negative remainder, which the
 * generator clamps to Easy. */
int daily_challenge_difficulty(CalendarDate date);

/* The daily puzzle of date: generate_seeded_numbers(daily_challenge_difficulty
 * (date), daily_challenge_seed(date)). Does not touch rand(). Returns NULL on
 * allocation failure; release with free_number_generation. */
NumberGeneration* generate_daily_challenge(CalendarDate date);

/* Writes "YYYY-MM-DD" (zero-padded). Returns false, writing nothing, when out
 * is NULL or size < TETONOR_DATE_TEXT_SIZE. */
bool format_calendar_date(CalendarDate date, char* out, size_t size);

#endif
