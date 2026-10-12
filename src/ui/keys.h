#ifndef TETONOR_UI_KEYS_H
#define TETONOR_UI_KEYS_H

#include "menu.h"
#include "../game/game_port.h"

enum
{
    TETONOR_KEY_ESCAPE    = 27,
    TETONOR_KEY_SPACE     = ' ',
    TETONOR_KEY_NEWLINE   = '\n',
    TETONOR_KEY_RETURN    = '\r',
    TETONOR_KEY_BACKSPACE = '\b',
    TETONOR_KEY_DELETE    = 127, /* DEL, sent as backspace by many terminals */
    TETONOR_KEY_DIGIT_FIRST = '0',
    TETONOR_KEY_DIGIT_LAST  = '9',
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
} BoardKeyCodes;

typedef enum
{
    BOARD_COMMAND_NONE, /* key ignored */
    BOARD_COMMAND_EXIT,
    BOARD_COMMAND_INPUT
} BoardCommandKind;

typedef struct
{
    BoardCommandKind kind;
    BoardInput input; /* meaningful only for BOARD_COMMAND_INPUT; else zeroed */
} BoardCommand;

/* Maps a key to a menu action. enter_key is the curses KEY_ENTER value, passed
 * in so this module stays free of curses. */
MenuResult menu_result_from_key(int key, int enter_key);

/* Maps a key to a board command. Unknown keys give BOARD_COMMAND_NONE. */
BoardCommand board_command_from_key(int key, const BoardKeyCodes* key_codes);

#endif
