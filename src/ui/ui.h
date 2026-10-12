#ifndef TETONOR_UI_H
#define TETONOR_UI_H

#include "../game/game_port.h"

/* Runs one puzzle until ESC. Returns EXIT_SUCCESS or EXIT_FAILURE. */
int ui_run(const GamePort* game_port);

#endif
