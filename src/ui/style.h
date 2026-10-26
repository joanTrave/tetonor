#ifndef TETONOR_UI_STYLE_H
#define TETONOR_UI_STYLE_H

#include <curses.h>

#include "../game/game_port.h"

/* Starts colour support when the terminal has it. Call once, after initscr;
 * curses releases colour state in endwin, so there is no matching release
 * function. */
void init_styles(void);

/* Defines one colour pair per palette role and paints the screen background
 * with the text colours. Call after init_styles and whenever the theme
 * changes; the next refresh repaints everything. */
void apply_palette_styles(const Palette* palette);

/* apply_palette_styles for theme's palette (the default for a value that is
 * not a theme). */
void apply_theme_styles(int theme);

/* Gives win the text colours and background of the current theme. Call on
 * every window right after creating it. */
void apply_window_style(WINDOW* win);

/* Curses attributes for role: the role's colour pair (bright for the typed
 * used roles) when colour is available, otherwise a monochrome fallback. */
attr_t palette_role_attr(PaletteRole role);

#endif
