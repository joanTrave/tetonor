#ifndef TETONOR_UI_STYLE_H
#define TETONOR_UI_STYLE_H

#include <curses.h>

typedef enum
{
    TEXT_STYLE_ENTERED, /* player-written numbers */
    TEXT_STYLE_CORRECT,
    TEXT_STYLE_WRONG,
    TEXT_STYLE_COUNT
} TextStyle;

/* Defines the colour pairs when the terminal supports colour. Call once,
 * after initscr; curses releases colour state in endwin, so there is no
 * matching release function. */
void init_styles(void);

/* Curses attributes for style: colour when available, otherwise a
 * monochrome fallback. */
attr_t text_style_attr(TextStyle style);

#endif
