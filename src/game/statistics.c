#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "statistics.h"
#include "game_timer.h"
#include "text_format.h"

#define MAX_NUMBER_DIGITS 18 /* 18 digits always fit an int64_t */

/* The only format version; any other header is rejected (no migration). */
static const char file_header[]                      = "TETONOR-STATS 1";

static const char* const file_keys[DIFFICULTY_COUNT] = {
    [DIFFICULTY_EASY]   = "easy",
    [DIFFICULTY_MEDIUM] = "medium",
    [DIFFICULTY_HARD]   = "hard",
};

void init_statistics(Statistics* statistics)
{
    memset(statistics, 0, sizeof *statistics);
}

bool record_completed_game(Statistics* statistics, int difficulty,
                           int32_t seconds)
{
    if (difficulty < 0 || difficulty >= DIFFICULTY_COUNT)
    {
        return false;
    }
    DifficultyStatistics* entry = &statistics->by_difficulty[difficulty];
    if (entry->completed_count == TETONOR_STATS_MAX_COMPLETED)
    {
        return false;
    }
    if (seconds < 0)
    {
        seconds = 0;
    }
    if (seconds > TETONOR_TIMER_MAX_SECONDS)
    {
        seconds = TETONOR_TIMER_MAX_SECONDS;
    }
    if (entry->completed_count == 0 || seconds < entry->best_seconds)
    {
        entry->best_seconds = seconds;
    }
    entry->completed_count++;
    entry->total_seconds += seconds;
    return true;
}

bool statistics_average_seconds(const DifficultyStatistics* entry,
                                int32_t* average)
{
    if (entry->completed_count <= 0)
    {
        return false;
    }
    int64_t count = entry->completed_count;
    *average      = (int32_t)((entry->total_seconds + count / 2) / count);
    return true;
}

bool serialize_statistics(const Statistics* statistics, char* out, size_t size)
{
    size_t used = 0;
    if (size == 0 || !advance_text_output(
                         snprintf(out, size, "%s\n", file_header), size, &used))
    {
        return false;
    }
    for (int i = 0; i < DIFFICULTY_COUNT; i++)
    {
        const DifficultyStatistics* entry = &statistics->by_difficulty[i];
        int written = snprintf(out + used, size - used,
                               "%s %" PRId32 " %" PRId32 " %" PRId64 "\n",
                               file_keys[i], entry->completed_count,
                               entry->best_seconds, entry->total_seconds);
        if (!advance_text_output(written, size, &used))
        {
            return false;
        }
    }
    return true;
}

static bool entry_is_consistent(const DifficultyStatistics* entry)
{
    if (entry->completed_count == 0)
    {
        return entry->best_seconds == 0 && entry->total_seconds == 0;
    }
    int64_t count = entry->completed_count;
    return entry->total_seconds >= entry->best_seconds * count &&
           entry->total_seconds <= count * TETONOR_TIMER_MAX_SECONDS;
}

/* "<key> <completed> <best> <total>" without its line end. */
static bool read_entry(TextCursor* cursor, const char* key,
                       DifficultyStatistics* entry)
{
    int64_t numbers[3];
    if (!read_text_literal(cursor, key))
    {
        return false;
    }
    for (size_t i = 0; i < 3; i++)
    {
        if (!read_text_literal(cursor, " ") ||
            !read_text_number(cursor, MAX_NUMBER_DIGITS, &numbers[i]))
        {
            return false;
        }
    }
    if (numbers[0] > TETONOR_STATS_MAX_COMPLETED ||
        numbers[1] > TETONOR_TIMER_MAX_SECONDS)
    {
        return false;
    }
    entry->completed_count = (int32_t)numbers[0];
    entry->best_seconds    = (int32_t)numbers[1];
    entry->total_seconds   = numbers[2];
    return entry_is_consistent(entry);
}

bool parse_statistics(const char* text, size_t length, Statistics* statistics)
{
    if (length > TETONOR_STATS_TEXT_MAX)
    {
        return false;
    }
    TextCursor cursor = make_text_cursor(text, length);
    Statistics parsed;
    if (!read_text_literal(&cursor, file_header) ||
        !read_text_line_end(&cursor))
    {
        return false;
    }
    for (int i = 0; i < DIFFICULTY_COUNT; i++)
    {
        bool last = i == DIFFICULTY_COUNT - 1;
        if (!read_entry(&cursor, file_keys[i], &parsed.by_difficulty[i]))
        {
            return false;
        }
        /* The last line may omit its line end. */
        if (!(last && is_text_cursor_at_end(&cursor)) &&
            !read_text_line_end(&cursor))
        {
            return false;
        }
    }
    if (!is_text_cursor_at_end(&cursor))
    {
        return false;
    }
    *statistics = parsed;
    return true;
}
