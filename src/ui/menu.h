#ifndef TETONOR_UI_MENU_H
#define TETONOR_UI_MENU_H

#include "../storage/config_store.h"
#include "../storage/saved_game_store.h"
#include "../storage/stats_store.h"

typedef enum
{
    MENU_START,
    MENU_EXIT,
    MENU_NONE, /* key ignored */
    MENU_STATISTICS,
    MENU_THEME,
    MENU_CONTINUE /* continue the saved game */
} MenuResult;

/* Shows the start menu, then the difficulty screen and a game, repeatedly,
 * until ESC on the start menu. The statistics screen opens from the start
 * menu; completed games are recorded in stats_store. The theme screen opens
 * from the start menu; the choice is applied at once and saved in
 * config_store. C continues the saved game of saved_game_store when it has
 * one. Returns EXIT_SUCCESS or EXIT_FAILURE. */
int ui_show_start_menu(StatsStore* stats_store, ConfigStore* config_store,
                       SavedGameStore* saved_game_store);

#endif
