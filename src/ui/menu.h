#ifndef TETONOR_UI_MENU_H
#define TETONOR_UI_MENU_H

typedef enum
{
    MENU_START,
    MENU_EXIT,
    MENU_NONE /* key ignored */
} MenuResult;

/* Shows the start menu and runs the game. Returns EXIT_SUCCESS or
 * EXIT_FAILURE. */
int ui_show_start_menu(void);

#endif
