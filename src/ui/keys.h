#ifndef TETONOR_UI_KEYS_H
#define TETONOR_UI_KEYS_H

#include "menu.h"

enum
{
    TETONOR_KEY_ESCAPE  = 27,
    TETONOR_KEY_SPACE   = ' ',
    TETONOR_KEY_NEWLINE = '\n',
    TETONOR_KEY_RETURN  = '\r'
};

/* Maps a key to a menu action. enter_key is the curses KEY_ENTER value, passed
 * in so this module stays free of curses. */
MenuResult menu_result_from_key(int key, int enter_key);

#endif
