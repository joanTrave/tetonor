#ifndef TETONOR_UI_STYLE_H
#define TETONOR_UI_STYLE_H

#include <curses.h>

#include "../game/game_port.h"

/* Starts colour support when the terminal has it. Call once, after initscr;
 * curses releases colour state in endwin, so there is no matching release
 * function. */
void init_styles(void);

/* Defines one colour pair per palette role. Call once per play session, after
 * init_styles and before the first draw. */
void apply_palette_styles(const Palette* palette);

/* Curses attributes for role: the role's colour pair (bright for the typed
 * used roles) when colour is available, otherwise a monochrome fallback. */
attr_t palette_role_attr(PaletteRole role);

#endif
