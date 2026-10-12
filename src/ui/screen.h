#ifndef TETONOR_UI_SCREEN_H
#define TETONOR_UI_SCREEN_H

/* Starts curses mode. Call once, from the composition root. */
void init_screen(void);

/* Leaves curses mode. Call once on every exit path after init_screen. */
void end_screen(void);

#endif
