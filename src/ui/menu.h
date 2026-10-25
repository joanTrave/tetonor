#ifndef TETONOR_UI_MENU_H
#define TETONOR_UI_MENU_H

#include "../storage/stats_store.h"

typedef enum
{
    MENU_START,
    MENU_EXIT,
    MENU_NONE, /* key ignored */
    MENU_STATISTICS
} MenuResult;

/* Shows the start menu, then the difficulty screen and a game, repeatedly,
 * until ESC on the start menu. The statistics screen opens from the start
 * menu; completed games are recorded in stats_store. Returns EXIT_SUCCESS or
 * EXIT_FAILURE. */
int ui_show_start_menu(StatsStore* stats_store);

#endif
