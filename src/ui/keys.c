#include <stdbool.h>

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

static bool key_matches(int key, int code)
{
    return code != TETONOR_KEY_UNAVAILABLE && key == code;
}

static BoardCommand make_board_command(BoardCommandKind kind,
                                       BoardInputKind input_kind, int digit)
{
    BoardCommand command;
    command.kind        = kind;
    command.input.kind  = input_kind;
    command.input.digit = digit;
    return command;
}

static BoardCommand input_command(BoardInputKind input_kind, int digit)
{
    return make_board_command(BOARD_COMMAND_INPUT, input_kind, digit);
}

BoardCommand board_command_from_key(int key, const BoardKeyCodes* key_codes)
{
    if (key == TETONOR_KEY_ESCAPE)
    {
        return make_board_command(BOARD_COMMAND_EXIT, 0, 0);
    }
    if (key >= TETONOR_KEY_DIGIT_FIRST && key <= TETONOR_KEY_DIGIT_LAST)
    {
        return input_command(BOARD_INPUT_DIGIT, key - TETONOR_KEY_DIGIT_FIRST);
    }
    if (key_matches(key, key_codes->pad_zero))
    {
        return input_command(BOARD_INPUT_DIGIT, 0);
    }
    if (key_matches(key, key_codes->left))
    {
        return input_command(BOARD_INPUT_MOVE_LEFT, 0);
    }
    if (key_matches(key, key_codes->right))
    {
        return input_command(BOARD_INPUT_MOVE_RIGHT, 0);
    }
    if (key_matches(key, key_codes->up))
    {
        return input_command(BOARD_INPUT_MOVE_UP, 0);
    }
    if (key_matches(key, key_codes->down))
    {
        return input_command(BOARD_INPUT_MOVE_DOWN, 0);
    }
    if (key == TETONOR_KEY_NEWLINE || key == TETONOR_KEY_RETURN ||
        key == TETONOR_KEY_SPACE || key_matches(key, key_codes->enter) ||
        key_matches(key, key_codes->pad_enter))
    {
        return input_command(BOARD_INPUT_ACTIVATE, 0);
    }
    if (key == TETONOR_KEY_BACKSPACE || key == TETONOR_KEY_DELETE ||
        key_matches(key, key_codes->backspace))
    {
        return input_command(BOARD_INPUT_DELETE, 0);
    }
    return make_board_command(BOARD_COMMAND_NONE, 0, 0);
}
