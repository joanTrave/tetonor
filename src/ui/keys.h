#ifndef TETONOR_UI_KEYS_H
#define TETONOR_UI_KEYS_H

#include "difficulty_menu.h"
#include "menu.h"
#include "pause_menu.h"
#include "stats_menu.h"
#include "theme_menu.h"
#include "victory_menu.h"
#include "../game/game_port.h"

enum
{
    TETONOR_KEY_ESCAPE    = 27,
    TETONOR_KEY_SPACE     = ' ',
    TETONOR_KEY_NEWLINE   = '\n',
    TETONOR_KEY_RETURN    = '\r',
    TETONOR_KEY_BACKSPACE = '\b',
    TETONOR_KEY_DELETE    = 127, /* DEL, sent as backspace by many terminals */
    TETONOR_KEY_DIGIT_FIRST      = '0',
    TETONOR_KEY_DIGIT_LAST       = '9',
    TETONOR_KEY_PLUS             = '+',
    TETONOR_KEY_MULTIPLY         = 'x',
    TETONOR_KEY_MULTIPLY_UPPER   = 'X',
    TETONOR_KEY_STAR             = '*',
    TETONOR_KEY_UNDO             = 'u',
    TETONOR_KEY_UNDO_UPPER       = 'U',
    TETONOR_KEY_STATISTICS       = 's',
    TETONOR_KEY_STATISTICS_UPPER = 'S',
    TETONOR_KEY_SAVE             = 's',
    TETONOR_KEY_SAVE_UPPER       = 'S',
    TETONOR_KEY_CONTINUE         = 'c',
    TETONOR_KEY_CONTINUE_UPPER   = 'C',
    TETONOR_KEY_PAUSE            = 'p',
    TETONOR_KEY_PAUSE_UPPER      = 'P',
    TETONOR_KEY_THEME            = 't',
    TETONOR_KEY_THEME_UPPER      = 'T',
    TETONOR_KEY_NEW_GAME         = 'n',
    TETONOR_KEY_NEW_GAME_UPPER   = 'N',
    TETONOR_KEY_MAIN_MENU        = 'm',
    TETONOR_KEY_MAIN_MENU_UPPER  = 'M',
    TETONOR_KEY_YES              = 'y',
    TETONOR_KEY_YES_UPPER        = 'Y',
    TETONOR_KEY_NO               = 'n', /* prompt answer; see NEW_GAME */
    TETONOR_KEY_NO_UPPER         = 'N',
    TETONOR_KEY_UNAVAILABLE = -2 /* never returned by getch(); absent code */
};

/* Curses key codes the board needs, passed in so this module stays free of
 * curses. Codes the curses implementation lacks are TETONOR_KEY_UNAVAILABLE. */
typedef struct
{
    int enter;     /* KEY_ENTER */
    int pad_enter; /* PDCurses PADENTER */
    int left;      /* KEY_LEFT */
    int right;     /* KEY_RIGHT */
    int backspace; /* KEY_BACKSPACE */
    int pad_zero;  /* PDCurses PAD0 */
    int up;        /* KEY_UP */
    int down;      /* KEY_DOWN */
    int pad_plus;  /* PDCurses PADPLUS */
    int pad_star;  /* PDCurses PADSTAR */
} BoardKeyCodes;

typedef enum
{
    BOARD_COMMAND_NONE, /* key ignored */
    BOARD_COMMAND_EXIT,
    BOARD_COMMAND_INPUT,
    BOARD_COMMAND_UNDO,      /* revert the latest move */
    BOARD_COMMAND_PAUSE,     /* open the pause menu */
    BOARD_COMMAND_NEW_GAME,  /* start another game at the same difficulty */
    BOARD_COMMAND_MAIN_MENU, /* leave for the main menu */
    BOARD_COMMAND_THEME      /* open the theme screen */
} BoardCommandKind;

typedef struct
{
    BoardCommandKind kind;
    BoardInput input; /* meaningful only for BOARD_COMMAND_INPUT; else zeroed */
} BoardCommand;

/* Maps a key to a menu action. enter_key is the curses KEY_ENTER value, passed
 * in so this module stays free of curses. */
MenuResult menu_result_from_key(int key, int enter_key);

/* Curses key codes the menus need, passed in so this module stays free of
 * curses. */
typedef struct
{
    int enter; /* KEY_ENTER */
    int up;    /* KEY_UP */
    int down;  /* KEY_DOWN */
} MenuKeyCodes;

/* Maps a key to a difficulty-screen command. Unknown keys give
 * DIFFICULTY_COMMAND_NONE. */
DifficultyMenuCommand
difficulty_menu_command_from_key(int key, const MenuKeyCodes* key_codes);

/* Maps a key to a pause-menu command. Unknown keys give
 * PAUSE_MENU_COMMAND_NONE. */
PauseMenuCommand pause_menu_command_from_key(int key,
                                             const MenuKeyCodes* key_codes);

/* Maps a key to a confirmation-prompt command. Unknown keys give
 * PROMPT_COMMAND_NONE. */
PromptCommand prompt_command_from_key(int key, const MenuKeyCodes* key_codes);

/* Maps a key to a victory-screen command. Unknown keys give
 * VICTORY_MENU_COMMAND_NONE. */
VictoryMenuCommand victory_menu_command_from_key(int key,
                                                 const MenuKeyCodes* key_codes);

/* Maps a key to a board command. Unknown keys give BOARD_COMMAND_NONE. */
BoardCommand board_command_from_key(int key, const BoardKeyCodes* key_codes);

/* Maps a key to a theme-screen command. Unknown keys give
 * THEME_MENU_COMMAND_NONE. */
ThemeMenuCommand theme_menu_command_from_key(int key,
                                             const MenuKeyCodes* key_codes);

/* Maps a key to a statistics-screen command. Unknown keys give
 * STATS_MENU_COMMAND_NONE. */
StatsMenuCommand stats_menu_command_from_key(int key);

#endif
