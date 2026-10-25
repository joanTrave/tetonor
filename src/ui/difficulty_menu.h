#ifndef TETONOR_UI_DIFFICULTY_MENU_H
#define TETONOR_UI_DIFFICULTY_MENU_H

#include "../game/game_port.h"

typedef enum
{
    DIFFICULTY_COMMAND_NONE, /* key ignored */
    DIFFICULTY_COMMAND_UP,
    DIFFICULTY_COMMAND_DOWN,
    DIFFICULTY_COMMAND_CONFIRM,
    DIFFICULTY_COMMAND_BACK
} DifficultyMenuCommand;

typedef enum
{
    DIFFICULTY_MENU_OPEN,
    DIFFICULTY_MENU_CONFIRMED, /* start a game at selected */
    DIFFICULTY_MENU_CANCELLED  /* back to the main menu */
} DifficultyMenuStatus;

typedef struct
{
    Difficulty selected;
    DifficultyMenuStatus status;
} DifficultyMenu;

/* Medium pre-selected, open. Call on every visit: nothing carries over. */
void init_difficulty_menu(DifficultyMenu* menu);

/* Applies one command: UP and DOWN move the selection and stop at the ends,
 * CONFIRM and BACK close the menu. Ignored once the menu is closed. */
void apply_difficulty_menu_command(DifficultyMenu* menu,
                                   DifficultyMenuCommand command);

/* Clamps a value to the valid Difficulty range [EASY, HARD]. */
int difficulty_menu_clamp(int difficulty);

/* "Easy", "Medium" or "Hard". Values out of range are clamped. */
const char* difficulty_menu_label(int difficulty);

#endif
