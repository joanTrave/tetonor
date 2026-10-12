#include <curses.h>
#include <stdlib.h>

#include "ui.h"
#include "keys.h"
#include "print_layout.h"

static BoardKeyCodes board_key_codes(void);
static void draw_board(const Board* board);

int ui_run(const GamePort* game_port)
{
    NumberGeneration* number_generation =
        game_port->generate_random_numbers_f(game_port->difficulty);
    if (number_generation == NULL)
    {
        return EXIT_FAILURE;
    }

    Board board;
    game_port->init_board_f(&board, number_generation);
    BoardKeyCodes key_codes = board_key_codes();
    draw_board(&board);

    int key = getch();
    while (key != ERR)
    {
        BoardCommand command = board_command_from_key(key, &key_codes);
        if (command.kind == BOARD_COMMAND_EXIT)
        {
            break;
        }
        if (command.kind == BOARD_COMMAND_INPUT)
        {
            game_port->apply_board_input_f(&board, command.input);
        }
        draw_board(&board);
        key = getch();
    }

    game_port->free_number_generation_f(number_generation);
    return EXIT_SUCCESS;
}

/* The only feature-macro guards: PDCurses-only keypad codes. */
static BoardKeyCodes board_key_codes(void)
{
    BoardKeyCodes key_codes;
    key_codes.enter     = KEY_ENTER;
    key_codes.left      = KEY_LEFT;
    key_codes.right     = KEY_RIGHT;
    key_codes.backspace = KEY_BACKSPACE;
#ifdef PADENTER
    key_codes.pad_enter = PADENTER;
#else
    key_codes.pad_enter = TETONOR_KEY_UNAVAILABLE;
#endif
#ifdef PAD0
    key_codes.pad_zero = PAD0;
#else
    key_codes.pad_zero = TETONOR_KEY_UNAVAILABLE;
#endif
    return key_codes;
}

static void draw_board(const Board* board)
{
    erase();
    print_layout(board);
    refresh();
}
