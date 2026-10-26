#ifndef TETONOR_UI_THEME_MENU_H
#define TETONOR_UI_THEME_MENU_H

#include "../game/game_port.h"

#define THEME_MENU_ACTIVE_MARKER " (current)"

typedef enum
{
    THEME_MENU_COMMAND_NONE, /* key ignored */
    THEME_MENU_COMMAND_UP,
    THEME_MENU_COMMAND_DOWN,
    THEME_MENU_COMMAND_SELECT,
    THEME_MENU_COMMAND_BACK
} ThemeMenuCommand;

typedef enum
{
    THEME_MENU_OPEN,
    THEME_MENU_CHOSEN,   /* apply selected */
    THEME_MENU_CANCELLED /* keep the active theme */
} ThemeMenuStatus;

typedef struct
{
    PaletteTheme active;   /* in use when the menu opened; marked */
    PaletteTheme selected; /* highlighted */
    ThemeMenuStatus status;
} ThemeMenu;

/* Open, with active (the default theme when it is not a theme) marked and
 * highlighted. Call on every visit: nothing carries over. */
void init_theme_menu(ThemeMenu* menu, int active_theme);

/* Applies one command: UP and DOWN move the highlight and stop at the first
 * and last theme, SELECT chooses the highlighted theme, BACK cancels. Ignored
 * once the menu is closed. */
void apply_theme_menu_command(ThemeMenu* menu, ThemeMenuCommand command);

/* "Classic" or "Light"; out-of-range values are clamped. */
const char* theme_menu_label(int theme);

/* THEME_MENU_ACTIVE_MARKER for the active theme, "" for any other. */
const char* theme_menu_marker(const ThemeMenu* menu, int theme);

#endif
