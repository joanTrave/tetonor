#include <string.h>

#include "stats_menu.h"
#include "difficulty_menu.h"

static void set_no_time(char* out)
{
    memcpy(out, TETONOR_NO_TIME_TEXT, sizeof TETONOR_NO_TIME_TEXT);
}

void build_stats_menu_row(const Statistics* statistics, int difficulty,
                          StatsMenuRow* row)
{
    const DifficultyStatistics* entry =
        &statistics->by_difficulty[difficulty_menu_clamp(difficulty)];
    int32_t average      = 0;
    row->label           = difficulty_menu_label(difficulty);
    row->completed_count = entry->completed_count;
    if (statistics_average_seconds(entry, &average))
    {
        format_elapsed_time(entry->best_seconds, row->best, sizeof row->best);
        format_elapsed_time(average, row->average, sizeof row->average);
        return;
    }
    set_no_time(row->best);
    set_no_time(row->average);
}
