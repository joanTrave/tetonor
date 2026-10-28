#include <stdio.h>

#include "daily_challenge.h"

#define TETONOR_DAILY_DIFFICULTY_CYCLE 3
#define TM_YEAR_BASE 1900 /* struct tm counts years from 1900 */
#define TM_MONTH_BASE 1   /* struct tm months are 0..11 */
#define SEED_YEAR_FACTOR 10000u
#define SEED_MONTH_FACTOR 100u
#define MAX_YEAR 9999
#define MIN_MONTH 1
#define MAX_MONTH 12
#define MIN_DAY 1
#define MAX_DAY 31

static int clamp_int(int value, int low, int high);

bool utc_calendar_date(time_t now, CalendarDate* out)
{
    if (now == (time_t)-1)
    {
        return false;
    }
    const struct tm* utc = gmtime(&now);
    if (utc == NULL)
    {
        return false;
    }
    out->year  = utc->tm_year + TM_YEAR_BASE;
    out->month = utc->tm_mon + TM_MONTH_BASE;
    out->day   = utc->tm_mday;
    return true;
}

uint32_t daily_challenge_seed(CalendarDate date)
{
    return (uint32_t)date.year * SEED_YEAR_FACTOR +
           (uint32_t)date.month * SEED_MONTH_FACTOR + (uint32_t)date.day;
}

int daily_challenge_difficulty(CalendarDate date)
{
    switch (date.day % TETONOR_DAILY_DIFFICULTY_CYCLE)
    {
        case 1:
            return DIFFICULTY_MEDIUM;
        case 2:
            return DIFFICULTY_HARD;
        default:
            return DIFFICULTY_EASY;
    }
}

NumberGeneration* generate_daily_challenge(CalendarDate date)
{
    return generate_seeded_numbers(daily_challenge_difficulty(date),
                                   daily_challenge_seed(date));
}

bool format_calendar_date(CalendarDate date, char* out, size_t size)
{
    if (out == NULL || size < TETONOR_DATE_TEXT_SIZE)
    {
        return false;
    }
    int written =
        snprintf(out, size, "%04d-%02d-%02d", clamp_int(date.year, 0, MAX_YEAR),
                 clamp_int(date.month, MIN_MONTH, MAX_MONTH),
                 clamp_int(date.day, MIN_DAY, MAX_DAY));
    return written > 0;
}

static int clamp_int(int value, int low, int high)
{
    if (value < low)
    {
        return low;
    }
    return value > high ? high : value;
}
