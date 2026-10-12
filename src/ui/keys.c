#include "keys.h"

MenuResult menu_result_from_key(int key, int enter_key)
{
    if (key == TETONOR_KEY_SPACE || key == TETONOR_KEY_NEWLINE ||
        key == TETONOR_KEY_RETURN || key == enter_key)
    {
        return MENU_START;
    }
    if (key == TETONOR_KEY_ESCAPE)
    {
        return MENU_EXIT;
    }
    return MENU_NONE;
}
