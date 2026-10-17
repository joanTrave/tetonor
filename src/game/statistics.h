#ifndef TETONOR_STATISTICS_H
#define TETONOR_STATISTICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "number_generation.h"

#define TETONOR_STATS_MAX_COMPLETED INT32_MAX
#define TETONOR_STATS_TEXT_MAX 256 /* longest stats file accepted, in bytes */

typedef struct
{
    int32_t completed_count;
    int32_t best_seconds;  /* meaningful only when completed_count > 0 */
    int64_t total_seconds; /* sum of every completed game's time */
} DifficultyStatistics;

/* Completed games per difficulty. A fixed-size value: nothing to release. */
typedef struct
{
    DifficultyStatistics by_difficulty[DIFFICULTY_COUNT];
} Statistics;

/* Empty statistics: every field zero. */
void init_statistics(Statistics* statistics);

/* Adds one completed game (seconds clamped to 0..TETONOR_TIMER_MAX_SECONDS).
 * Returns false, changing nothing, for a difficulty out of range or a count
 * already at TETONOR_STATS_MAX_COMPLETED. */
bool record_completed_game(Statistics* statistics, int difficulty,
                           int32_t seconds);

/* Total divided by count, rounded half up. Returns false, leaving *average
 * untouched, when no game was completed. */
bool statistics_average_seconds(const DifficultyStatistics* entry,
                                int32_t* average);

/* Writes the versioned text format, terminated by '\0'. Returns false if it
 * does not fit size. */
bool serialize_statistics(const Statistics* statistics, char* out, size_t size);

/* Parses exactly length bytes. Returns false, leaving *statistics untouched,
 * on any malformed, out-of-range or wrong-version input. */
bool parse_statistics(const char* text, size_t length, Statistics* statistics);

#endif
