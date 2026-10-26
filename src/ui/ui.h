#ifndef TETONOR_UI_H
#define TETONOR_UI_H

#include "../game/game_port.h"
#include "../game/saved_game.h"
#include "../storage/config_store.h"
#include "../storage/saved_game_store.h"
#include "../storage/stats_store.h"

/* Runs one puzzle until the player leaves it (ESC, M, Abandon, or Main menu on
 * the victory screen); records a completed game in stats_store and a chosen
 * theme in config_store. A new puzzle starts when resume is NULL; otherwise the
 * game continues from resume, which is read only while the game starts. The
 * pause menu saves the game into saved_game_store, and completing a game that
 * is the saved one discards the save. Returns EXIT_SUCCESS or EXIT_FAILURE. */
int ui_run(const GamePort* game_port, StatsStore* stats_store,
           ConfigStore* config_store, SavedGameStore* saved_game_store,
           const SavedGame* resume);

/* Applies theme to every screen now and saves it in config_store; a failed
 * save is ignored (the theme still applies for the session). */
void ui_apply_theme(ConfigStore* config_store, int theme);

#endif
