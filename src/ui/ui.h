#ifndef TETONOR_UI_H
#define TETONOR_UI_H

#include "../game/game_port.h"

/* Draws a puzzle and waits for ESC. Returns EXIT_SUCCESS or EXIT_FAILURE. */
int ui_run(const GamePort* game_port);

#endif
