#ifndef TETONOR_UI_H
#define TETONOR_UI_H

#include "../game/game_port.h"
#include "../storage/stats_store.h"

/* Runs one puzzle until the player leaves it (ESC, M or Abandon); records a
 * completed game in stats_store. Returns EXIT_SUCCESS or EXIT_FAILURE. */
int ui_run(const GamePort* game_port, StatsStore* stats_store);

#endif
