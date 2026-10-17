#ifndef TETONOR_UI_STATS_MENU_H
#define TETONOR_UI_STATS_MENU_H

#include <stdint.h>

#include "../game/game_port.h"

#define TETONOR_NO_TIME_TEXT "--:--"

typedef enum
{
    STATS_MENU_COMMAND_NONE, /* key ignored */
    STATS_MENU_COMMAND_BACK
} StatsMenuCommand;

/* One display row of the statistics screen. */
typedef struct
{
    const char* label; /* difficulty_menu_label */
    int32_t completed_count;
    char best[TETONOR_ELAPSED_TEXT_SIZE]; /* TETONOR_NO_TIME_TEXT when none */
    char
        average[TETONOR_ELAPSED_TEXT_SIZE]; /* TETONOR_NO_TIME_TEXT when none */
} StatsMenuRow;

/* Fills row for difficulty (clamped like difficulty_menu_label). */
void build_stats_menu_row(const Statistics* statistics, int difficulty,
                          StatsMenuRow* row);

#endif
