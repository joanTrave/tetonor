#include <stdbool.h>
#include <stddef.h>

#include "../test_harness.h"
#include "../../src/ui/keys.h"

/* Stands in for curses KEY_ENTER: its value differs between ncurses and
 * PDCurses, and tests must not include curses. */
#define TEST_ENTER_KEY 0x157
#define TEST_ESCAPE_LITERAL 27
#define TEST_ARROW_KEY 0x102

static void test_menu_result_from_key_space_starts(void)
{
    ASSERT_EQ_INT(menu_result_from_key(' ', TEST_ENTER_KEY), MENU_START);
}

static void test_menu_result_from_key_newline_starts(void)
{
    ASSERT_EQ_INT(menu_result_from_key('\n', TEST_ENTER_KEY), MENU_START);
}

static void test_menu_result_from_key_carriage_return_starts(void)
{
    ASSERT_EQ_INT(menu_result_from_key('\r', TEST_ENTER_KEY), MENU_START);
}

static void test_menu_result_from_key_enter_key_starts(void)
{
    ASSERT_EQ_INT(menu_result_from_key(TEST_ENTER_KEY, TEST_ENTER_KEY),
                  MENU_START);
}

static void test_menu_result_from_key_escape_exits(void)
{
    ASSERT_EQ_INT(menu_result_from_key(TETONOR_KEY_ESCAPE, TEST_ENTER_KEY),
                  MENU_EXIT);
    ASSERT_EQ_INT(menu_result_from_key(TEST_ESCAPE_LITERAL, TEST_ENTER_KEY),
                  MENU_EXIT);
}

static void test_menu_result_from_key_unknown_key_is_ignored(void)
{
    const int keys[] = {'a', 0, -1, TEST_ARROW_KEY};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(menu_result_from_key(keys[i], TEST_ENTER_KEY), MENU_NONE);
    }
}

#define TEST_PAD_ENTER_KEY 0x1cb
#define TEST_LEFT_KEY 0x104
#define TEST_RIGHT_KEY 0x105
#define TEST_BACKSPACE_KEY 0x107
#define TEST_PAD_ZERO_KEY 0x1fa
#define TEST_UP_KEY 0x103
#define TEST_DOWN_KEY 0x102
#define TEST_PAD_PLUS_KEY 0x1d1
#define TEST_PAD_STAR_KEY 0x1cf
#define TEST_HOME_KEY 0x106
#define TEST_PAGE_DOWN_KEY 0x152
#define TEST_RESIZE_KEY 0x19a
#define TEST_ERR_KEY (-1)

static const BoardKeyCodes test_codes        = {.enter     = TEST_ENTER_KEY,
                                                .pad_enter = TEST_PAD_ENTER_KEY,
                                                .left      = TEST_LEFT_KEY,
                                                .right     = TEST_RIGHT_KEY,
                                                .backspace = TEST_BACKSPACE_KEY,
                                                .pad_zero  = TEST_PAD_ZERO_KEY,
                                                .up        = TEST_UP_KEY,
                                                .down      = TEST_DOWN_KEY,
                                                .pad_plus  = TEST_PAD_PLUS_KEY,
                                                .pad_star  = TEST_PAD_STAR_KEY};

static const BoardKeyCodes unavailable_codes = {
    .enter     = TEST_ENTER_KEY,
    .pad_enter = TETONOR_KEY_UNAVAILABLE,
    .left      = TEST_LEFT_KEY,
    .right     = TEST_RIGHT_KEY,
    .backspace = TEST_BACKSPACE_KEY,
    .pad_zero  = TETONOR_KEY_UNAVAILABLE,
    .up        = TEST_UP_KEY,
    .down      = TEST_DOWN_KEY,
    .pad_plus  = TETONOR_KEY_UNAVAILABLE,
    .pad_star  = TETONOR_KEY_UNAVAILABLE};

static const BoardKeyCodes vertical_unavailable_codes = {
    .enter     = TEST_ENTER_KEY,
    .pad_enter = TEST_PAD_ENTER_KEY,
    .left      = TEST_LEFT_KEY,
    .right     = TEST_RIGHT_KEY,
    .backspace = TEST_BACKSPACE_KEY,
    .pad_zero  = TEST_PAD_ZERO_KEY,
    .up        = TETONOR_KEY_UNAVAILABLE,
    .down      = TETONOR_KEY_UNAVAILABLE,
    .pad_plus  = TEST_PAD_PLUS_KEY,
    .pad_star  = TEST_PAD_STAR_KEY};

#define ASSERT_INPUT_COMMAND(key, codes, expected_kind)                        \
    do                                                                         \
    {                                                                          \
        BoardCommand command_under_test =                                      \
            board_command_from_key((key), &(codes));                           \
        ASSERT_EQ_INT(command_under_test.kind, BOARD_COMMAND_INPUT);           \
        ASSERT_EQ_INT(command_under_test.input.kind, (expected_kind));         \
        ASSERT_EQ_INT(command_under_test.input.operation,                      \
                      GROUP_OPERATION_NONE);                                   \
    } while (0)

#define ASSERT_DIGIT_COMMAND(key, codes, expected_digit)                       \
    do                                                                         \
    {                                                                          \
        BoardCommand command_under_test =                                      \
            board_command_from_key((key), &(codes));                           \
        ASSERT_EQ_INT(command_under_test.kind, BOARD_COMMAND_INPUT);           \
        ASSERT_EQ_INT(command_under_test.input.kind, BOARD_INPUT_DIGIT);       \
        ASSERT_EQ_INT(command_under_test.input.digit, (expected_digit));       \
        ASSERT_EQ_INT(command_under_test.input.operation,                      \
                      GROUP_OPERATION_NONE);                                   \
    } while (0)

#define ASSERT_OPERATION_COMMAND(key, codes, expected_operation)               \
    do                                                                         \
    {                                                                          \
        BoardCommand command_under_test =                                      \
            board_command_from_key((key), &(codes));                           \
        ASSERT_EQ_INT(command_under_test.kind, BOARD_COMMAND_INPUT);           \
        ASSERT_EQ_INT(command_under_test.input.kind, BOARD_INPUT_OPERATION);   \
        ASSERT_EQ_INT(command_under_test.input.operation,                      \
                      (expected_operation));                                   \
    } while (0)

#define ASSERT_NO_COMMAND(key, codes)                                          \
    do                                                                         \
    {                                                                          \
        BoardCommand command_under_test =                                      \
            board_command_from_key((key), &(codes));                           \
        ASSERT_EQ_INT(command_under_test.kind, BOARD_COMMAND_NONE);            \
        ASSERT_EQ_INT(command_under_test.input.kind, 0);                       \
        ASSERT_EQ_INT(command_under_test.input.digit, 0);                      \
        ASSERT_EQ_INT(command_under_test.input.operation,                      \
                      GROUP_OPERATION_NONE);                                   \
    } while (0)

static void test_board_command_from_key_escape_exits(void)
{
    BoardCommand command =
        board_command_from_key(TETONOR_KEY_ESCAPE, &test_codes);
    ASSERT_EQ_INT(command.kind, BOARD_COMMAND_EXIT);
    ASSERT_EQ_INT(command.input.kind, 0);
    ASSERT_EQ_INT(command.input.digit, 0);
    ASSERT_EQ_INT(command.input.operation, GROUP_OPERATION_NONE);
    command = board_command_from_key(TEST_ESCAPE_LITERAL, &test_codes);
    ASSERT_EQ_INT(command.kind, BOARD_COMMAND_EXIT);
}

static void test_board_command_from_key_arrows_move_cursor(void)
{
    ASSERT_INPUT_COMMAND(TEST_LEFT_KEY, test_codes, BOARD_INPUT_MOVE_LEFT);
    ASSERT_INPUT_COMMAND(TEST_RIGHT_KEY, test_codes, BOARD_INPUT_MOVE_RIGHT);
}

static void test_board_command_from_key_main_row_digits_type_digit(void)
{
    ASSERT_DIGIT_COMMAND('0', test_codes, 0);
    ASSERT_DIGIT_COMMAND('5', test_codes, 5);
    ASSERT_DIGIT_COMMAND('9', test_codes, 9);
}

static void test_board_command_from_key_every_digit_maps_to_its_value(void)
{
    for (int digit = 0; digit < TETONOR_DIGIT_BASE; digit++)
    {
        ASSERT_DIGIT_COMMAND(TETONOR_KEY_DIGIT_FIRST + digit, test_codes,
                             digit);
    }
}

static void test_board_command_from_key_numpad_zero_types_zero(void)
{
    ASSERT_DIGIT_COMMAND(TEST_PAD_ZERO_KEY, test_codes, 0);
}

static void test_board_command_from_key_arrows_move_focus_vertically(void)
{
    ASSERT_INPUT_COMMAND(TEST_UP_KEY, test_codes, BOARD_INPUT_MOVE_UP);
    ASSERT_INPUT_COMMAND(TEST_DOWN_KEY, test_codes, BOARD_INPUT_MOVE_DOWN);
}

static void test_board_command_from_key_enter_variants_activate(void)
{
    ASSERT_INPUT_COMMAND('\n', test_codes, BOARD_INPUT_ACTIVATE);
    ASSERT_INPUT_COMMAND('\r', test_codes, BOARD_INPUT_ACTIVATE);
    ASSERT_INPUT_COMMAND(TEST_ENTER_KEY, test_codes, BOARD_INPUT_ACTIVATE);
    ASSERT_INPUT_COMMAND(TEST_PAD_ENTER_KEY, test_codes, BOARD_INPUT_ACTIVATE);
}

static void test_board_command_from_key_backspace_variants_delete(void)
{
    ASSERT_INPUT_COMMAND('\b', test_codes, BOARD_INPUT_DELETE);
    ASSERT_INPUT_COMMAND(TETONOR_KEY_DELETE, test_codes, BOARD_INPUT_DELETE);
    ASSERT_INPUT_COMMAND(127, test_codes, BOARD_INPUT_DELETE);
    ASSERT_INPUT_COMMAND(TEST_BACKSPACE_KEY, test_codes, BOARD_INPUT_DELETE);
}

static void test_board_command_from_key_space_activates(void)
{
    ASSERT_INPUT_COMMAND(' ', test_codes, BOARD_INPUT_ACTIVATE);
    ASSERT_INPUT_COMMAND(TETONOR_KEY_SPACE, unavailable_codes,
                         BOARD_INPUT_ACTIVATE);
}

static void test_board_command_from_key_plus_adds(void)
{
    ASSERT_OPERATION_COMMAND('+', test_codes, GROUP_OPERATION_ADD);
}

static void test_board_command_from_key_keypad_plus_adds(void)
{
    ASSERT_OPERATION_COMMAND(TEST_PAD_PLUS_KEY, test_codes,
                             GROUP_OPERATION_ADD);
}

static void test_board_command_from_key_multiplication_keys_multiply(void)
{
    ASSERT_OPERATION_COMMAND('x', test_codes, GROUP_OPERATION_MULTIPLY);
    ASSERT_OPERATION_COMMAND('X', test_codes, GROUP_OPERATION_MULTIPLY);
    ASSERT_OPERATION_COMMAND('*', test_codes, GROUP_OPERATION_MULTIPLY);
}

static void test_board_command_from_key_keypad_star_multiplies(void)
{
    ASSERT_OPERATION_COMMAND(TEST_PAD_STAR_KEY, test_codes,
                             GROUP_OPERATION_MULTIPLY);
}

static void test_board_command_from_key_operation_characters_ignore_codes(void)
{
    ASSERT_OPERATION_COMMAND('+', unavailable_codes, GROUP_OPERATION_ADD);
    ASSERT_OPERATION_COMMAND('x', unavailable_codes, GROUP_OPERATION_MULTIPLY);
    ASSERT_OPERATION_COMMAND('X', unavailable_codes, GROUP_OPERATION_MULTIPLY);
    ASSERT_OPERATION_COMMAND('*', unavailable_codes, GROUP_OPERATION_MULTIPLY);
}

static void test_board_command_from_key_other_operators_are_ignored(void)
{
    const int keys[] = {'-', '/', '=', 'y', 'Y', '%', '^', 'v'};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_NO_COMMAND(keys[i], test_codes);
    }
}

static void test_board_command_from_key_digit_range_neighbours_are_ignored(void)
{
    ASSERT_NO_COMMAND('/', test_codes);
    ASSERT_NO_COMMAND(':', test_codes);
}

static void test_board_command_from_key_unrelated_keys_are_ignored(void)
{
    const int keys[] = {'a',
                        'z',
                        0,
                        TEST_ERR_KEY,
                        TEST_RESIZE_KEY,
                        TEST_HOME_KEY,
                        TEST_PAGE_DOWN_KEY};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_NO_COMMAND(keys[i], test_codes);
    }
}

static void test_board_command_from_key_unavailable_codes_never_match(void)
{
    ASSERT_NO_COMMAND(TETONOR_KEY_UNAVAILABLE, unavailable_codes);
    ASSERT_NO_COMMAND(TEST_PAD_ENTER_KEY, unavailable_codes);
    ASSERT_NO_COMMAND(TEST_PAD_ZERO_KEY, unavailable_codes);
    ASSERT_NO_COMMAND(TEST_PAD_PLUS_KEY, unavailable_codes);
    ASSERT_NO_COMMAND(TEST_PAD_STAR_KEY, unavailable_codes);
}

static void
test_board_command_from_key_unavailable_vertical_codes_never_match(void)
{
    ASSERT_NO_COMMAND(TETONOR_KEY_UNAVAILABLE, vertical_unavailable_codes);
    ASSERT_NO_COMMAND(TEST_UP_KEY, vertical_unavailable_codes);
    ASSERT_NO_COMMAND(TEST_DOWN_KEY, vertical_unavailable_codes);
    ASSERT_INPUT_COMMAND(TEST_LEFT_KEY, vertical_unavailable_codes,
                         BOARD_INPUT_MOVE_LEFT);
    ASSERT_INPUT_COMMAND(TEST_ENTER_KEY, vertical_unavailable_codes,
                         BOARD_INPUT_ACTIVATE);
    ASSERT_OPERATION_COMMAND(TEST_PAD_PLUS_KEY, vertical_unavailable_codes,
                             GROUP_OPERATION_ADD);
    ASSERT_OPERATION_COMMAND(TEST_PAD_STAR_KEY, vertical_unavailable_codes,
                             GROUP_OPERATION_MULTIPLY);
}

static void test_board_command_from_key_available_codes_still_work(void)
{
    ASSERT_INPUT_COMMAND(TEST_ENTER_KEY, unavailable_codes,
                         BOARD_INPUT_ACTIVATE);
    ASSERT_INPUT_COMMAND(TEST_UP_KEY, unavailable_codes, BOARD_INPUT_MOVE_UP);
    ASSERT_INPUT_COMMAND(TEST_DOWN_KEY, unavailable_codes,
                         BOARD_INPUT_MOVE_DOWN);
    ASSERT_INPUT_COMMAND(TEST_LEFT_KEY, unavailable_codes,
                         BOARD_INPUT_MOVE_LEFT);
    ASSERT_INPUT_COMMAND(TEST_RIGHT_KEY, unavailable_codes,
                         BOARD_INPUT_MOVE_RIGHT);
    ASSERT_INPUT_COMMAND(TEST_BACKSPACE_KEY, unavailable_codes,
                         BOARD_INPUT_DELETE);
    ASSERT_DIGIT_COMMAND('7', unavailable_codes, 7);
}

#define ASSERT_UNDO_COMMAND(key, codes)                                        \
    do                                                                         \
    {                                                                          \
        BoardCommand command_under_test =                                      \
            board_command_from_key((key), &(codes));                           \
        ASSERT_EQ_INT(command_under_test.kind, BOARD_COMMAND_UNDO);            \
        ASSERT_EQ_INT(command_under_test.input.kind, 0);                       \
        ASSERT_EQ_INT(command_under_test.input.digit, 0);                      \
        ASSERT_EQ_INT(command_under_test.input.operation,                      \
                      GROUP_OPERATION_NONE);                                   \
    } while (0)

static void test_board_command_from_key_undo_key_codes_are_u(void)
{
    ASSERT_EQ_INT(TETONOR_KEY_UNDO, 'u');
    ASSERT_EQ_INT(TETONOR_KEY_UNDO_UPPER, 'U');
}

static void test_board_command_from_key_u_undoes(void)
{
    ASSERT_UNDO_COMMAND('u', test_codes);
    ASSERT_UNDO_COMMAND('U', test_codes);
    ASSERT_UNDO_COMMAND('u', unavailable_codes);
    ASSERT_UNDO_COMMAND('U', unavailable_codes);
    ASSERT_UNDO_COMMAND('u', vertical_unavailable_codes);
    ASSERT_UNDO_COMMAND('U', vertical_unavailable_codes);
}

static void test_board_command_from_key_undo_neighbours_are_ignored(void)
{
    const int keys[] = {'v', 'V'};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_NO_COMMAND(keys[i], test_codes);
        ASSERT_NO_COMMAND(keys[i], unavailable_codes);
    }
}

static void test_board_command_from_key_undo_keeps_existing_mappings(void)
{
    BoardCommand escape =
        board_command_from_key(TETONOR_KEY_ESCAPE, &test_codes);
    ASSERT_EQ_INT(escape.kind, BOARD_COMMAND_EXIT);
    ASSERT_OPERATION_COMMAND('x', test_codes, GROUP_OPERATION_MULTIPLY);
    ASSERT_OPERATION_COMMAND('X', test_codes, GROUP_OPERATION_MULTIPLY);
    ASSERT_OPERATION_COMMAND('+', test_codes, GROUP_OPERATION_ADD);
    ASSERT_DIGIT_COMMAND('5', test_codes, 5);
    ASSERT_INPUT_COMMAND(TEST_ENTER_KEY, test_codes, BOARD_INPUT_ACTIVATE);
    ASSERT_INPUT_COMMAND(TEST_BACKSPACE_KEY, test_codes, BOARD_INPUT_DELETE);
}

static void test_menus_do_not_react_to_the_undo_key(void)
{
    const int keys[] = {'u', 'U'};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(menu_result_from_key(keys[i], TEST_ENTER_KEY), MENU_NONE);
    }
}

static void test_menu_result_from_key_s_opens_statistics(void)
{
    ASSERT_EQ_INT(menu_result_from_key('s', TEST_ENTER_KEY), MENU_STATISTICS);
    ASSERT_EQ_INT(menu_result_from_key('S', TEST_ENTER_KEY), MENU_STATISTICS);
    ASSERT_EQ_INT(menu_result_from_key(TETONOR_KEY_STATISTICS, TEST_ENTER_KEY),
                  MENU_STATISTICS);
    ASSERT_EQ_INT(
        menu_result_from_key(TETONOR_KEY_STATISTICS_UPPER, TEST_ENTER_KEY),
        MENU_STATISTICS);
}

static void test_menu_result_from_key_existing_mappings_unchanged(void)
{
    ASSERT_EQ_INT(menu_result_from_key(' ', TEST_ENTER_KEY), MENU_START);
    ASSERT_EQ_INT(menu_result_from_key('\n', TEST_ENTER_KEY), MENU_START);
    ASSERT_EQ_INT(menu_result_from_key(TEST_ENTER_KEY, TEST_ENTER_KEY),
                  MENU_START);
    ASSERT_EQ_INT(menu_result_from_key(TETONOR_KEY_ESCAPE, TEST_ENTER_KEY),
                  MENU_EXIT);
    ASSERT_EQ_INT(menu_result_from_key('a', TEST_ENTER_KEY), MENU_NONE);
    ASSERT_EQ_INT(MENU_START, 0);
    ASSERT_EQ_INT(MENU_EXIT, 1);
    ASSERT_EQ_INT(MENU_NONE, 2);
}

static void test_menu_result_from_key_neighbours_of_s_are_ignored(void)
{
    const int keys[] = {'r', 'R', 'z'};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(menu_result_from_key(keys[i], TEST_ENTER_KEY), MENU_NONE);
    }
}

static void test_board_command_from_key_s_is_ignored_in_game(void)
{
    const int keys[] = {'s', 'S'};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        BoardCommand command = board_command_from_key(keys[i], &test_codes);
        ASSERT_EQ_INT(command.kind, BOARD_COMMAND_NONE);
    }
}

static void test_difficulty_menu_command_from_key_ignores_the_s_key(void)
{
    const MenuKeyCodes codes = {
        .enter = TEST_ENTER_KEY, .up = TEST_UP_KEY, .down = TEST_DOWN_KEY};
    ASSERT_EQ_INT(difficulty_menu_command_from_key('s', &codes),
                  DIFFICULTY_COMMAND_NONE);
    ASSERT_EQ_INT(difficulty_menu_command_from_key('S', &codes),
                  DIFFICULTY_COMMAND_NONE);
}

static void test_stats_menu_command_from_key(void)
{
    ASSERT_EQ_INT(stats_menu_command_from_key(TETONOR_KEY_ESCAPE),
                  STATS_MENU_COMMAND_BACK);
    const int ignored[] = {'s', 'S', ' ', '\n', '\r', 'q', 'u', 0, -1};
    for (size_t i = 0; i < sizeof ignored / sizeof ignored[0]; i++)
    {
        ASSERT_EQ_INT(stats_menu_command_from_key(ignored[i]),
                      STATS_MENU_COMMAND_NONE);
    }
}

static const MenuKeyCodes menu_codes = {
    .enter = TEST_ENTER_KEY, .up = TEST_UP_KEY, .down = TEST_DOWN_KEY};

static const MenuKeyCodes unavailable_menu_codes = {
    .enter = TETONOR_KEY_UNAVAILABLE,
    .up    = TETONOR_KEY_UNAVAILABLE,
    .down  = TETONOR_KEY_UNAVAILABLE};

static void test_difficulty_menu_command_from_key_arrows_move(void)
{
    ASSERT_EQ_INT(difficulty_menu_command_from_key(TEST_UP_KEY, &menu_codes),
                  DIFFICULTY_COMMAND_UP);
    ASSERT_EQ_INT(difficulty_menu_command_from_key(TEST_DOWN_KEY, &menu_codes),
                  DIFFICULTY_COMMAND_DOWN);
}

static void test_difficulty_menu_command_from_key_enter_variants_confirm(void)
{
    const int keys[] = {' ', '\n', '\r', TEST_ENTER_KEY};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(difficulty_menu_command_from_key(keys[i], &menu_codes),
                      DIFFICULTY_COMMAND_CONFIRM);
    }
}

static void test_difficulty_menu_command_from_key_escape_goes_back(void)
{
    ASSERT_EQ_INT(
        difficulty_menu_command_from_key(TETONOR_KEY_ESCAPE, &menu_codes),
        DIFFICULTY_COMMAND_BACK);
    ASSERT_EQ_INT(
        difficulty_menu_command_from_key(TEST_ESCAPE_LITERAL, &menu_codes),
        DIFFICULTY_COMMAND_BACK);
}

static void
test_difficulty_menu_command_from_key_unrelated_keys_are_ignored(void)
{
    const int keys[] = {'a',
                        '1',
                        '+',
                        'x',
                        0,
                        TEST_ERR_KEY,
                        TEST_BACKSPACE_KEY,
                        TEST_LEFT_KEY,
                        TEST_RIGHT_KEY,
                        TEST_RESIZE_KEY};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(difficulty_menu_command_from_key(keys[i], &menu_codes),
                      DIFFICULTY_COMMAND_NONE);
    }
}

static void
test_difficulty_menu_command_from_key_unavailable_codes_never_match(void)
{
    ASSERT_EQ_INT(difficulty_menu_command_from_key(TETONOR_KEY_UNAVAILABLE,
                                                   &unavailable_menu_codes),
                  DIFFICULTY_COMMAND_NONE);
    ASSERT_EQ_INT(
        difficulty_menu_command_from_key(TEST_UP_KEY, &unavailable_menu_codes),
        DIFFICULTY_COMMAND_NONE);
    ASSERT_EQ_INT(difficulty_menu_command_from_key(TEST_DOWN_KEY,
                                                   &unavailable_menu_codes),
                  DIFFICULTY_COMMAND_NONE);
    ASSERT_EQ_INT(
        difficulty_menu_command_from_key(' ', &unavailable_menu_codes),
        DIFFICULTY_COMMAND_CONFIRM);
}

static void test_difficulty_menu_keys_select_hard_then_confirm(void)
{
    const int keys[] = {TEST_DOWN_KEY, TEST_DOWN_KEY, TEST_ENTER_KEY};
    DifficultyMenu menu;
    init_difficulty_menu(&menu);
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        apply_difficulty_menu_command(
            &menu, difficulty_menu_command_from_key(keys[i], &menu_codes));
    }
    ASSERT_EQ_INT(menu.status, DIFFICULTY_MENU_CONFIRMED);
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_HARD);
}

static void test_difficulty_menu_keys_escape_returns_without_starting(void)
{
    const int keys[] = {TEST_DOWN_KEY, TETONOR_KEY_ESCAPE};
    DifficultyMenu menu;
    init_difficulty_menu(&menu);
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        apply_difficulty_menu_command(
            &menu, difficulty_menu_command_from_key(keys[i], &menu_codes));
    }
    ASSERT_EQ_INT(menu.status, DIFFICULTY_MENU_CANCELLED);
}

static void test_difficulty_menu_keys_enter_on_fresh_menu_starts_medium(void)
{
    DifficultyMenu menu;
    init_difficulty_menu(&menu);
    apply_difficulty_menu_command(
        &menu, difficulty_menu_command_from_key(' ', &menu_codes));
    ASSERT_EQ_INT(menu.status, DIFFICULTY_MENU_CONFIRMED);
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_MEDIUM);
}

static void test_difficulty_menu_command_from_key_ignores_the_undo_key(void)
{
    ASSERT_EQ_INT(difficulty_menu_command_from_key('u', &menu_codes),
                  DIFFICULTY_COMMAND_NONE);
    ASSERT_EQ_INT(difficulty_menu_command_from_key('U', &menu_codes),
                  DIFFICULTY_COMMAND_NONE);
}

#define TEST_SCAN_LAST_KEY 1023

#define ASSERT_SIMPLE_COMMAND(key, codes, expected_kind)                       \
    do                                                                         \
    {                                                                          \
        BoardCommand command_under_test =                                      \
            board_command_from_key((key), &(codes));                           \
        ASSERT_EQ_INT(command_under_test.kind, (expected_kind));               \
        ASSERT_EQ_INT(command_under_test.input.kind, 0);                       \
        ASSERT_EQ_INT(command_under_test.input.digit, 0);                      \
        ASSERT_EQ_INT(command_under_test.input.operation,                      \
                      GROUP_OPERATION_NONE);                                   \
    } while (0)

static void test_board_command_from_key_p_pauses(void)
{
    ASSERT_SIMPLE_COMMAND('p', test_codes, BOARD_COMMAND_PAUSE);
    ASSERT_SIMPLE_COMMAND('P', test_codes, BOARD_COMMAND_PAUSE);
    ASSERT_SIMPLE_COMMAND('p', unavailable_codes, BOARD_COMMAND_PAUSE);
    ASSERT_SIMPLE_COMMAND('P', unavailable_codes, BOARD_COMMAND_PAUSE);
    ASSERT_SIMPLE_COMMAND('p', vertical_unavailable_codes, BOARD_COMMAND_PAUSE);
    ASSERT_SIMPLE_COMMAND('P', vertical_unavailable_codes, BOARD_COMMAND_PAUSE);
}

static void test_board_command_from_key_n_starts_new_game(void)
{
    ASSERT_SIMPLE_COMMAND('n', test_codes, BOARD_COMMAND_NEW_GAME);
    ASSERT_SIMPLE_COMMAND('N', test_codes, BOARD_COMMAND_NEW_GAME);
    ASSERT_SIMPLE_COMMAND('n', unavailable_codes, BOARD_COMMAND_NEW_GAME);
    ASSERT_SIMPLE_COMMAND('N', unavailable_codes, BOARD_COMMAND_NEW_GAME);
    ASSERT_SIMPLE_COMMAND('n', vertical_unavailable_codes,
                          BOARD_COMMAND_NEW_GAME);
    ASSERT_SIMPLE_COMMAND('N', vertical_unavailable_codes,
                          BOARD_COMMAND_NEW_GAME);
}

static void test_board_command_from_key_m_returns_to_main_menu(void)
{
    ASSERT_SIMPLE_COMMAND('m', test_codes, BOARD_COMMAND_MAIN_MENU);
    ASSERT_SIMPLE_COMMAND('M', test_codes, BOARD_COMMAND_MAIN_MENU);
    ASSERT_SIMPLE_COMMAND('m', unavailable_codes, BOARD_COMMAND_MAIN_MENU);
    ASSERT_SIMPLE_COMMAND('M', unavailable_codes, BOARD_COMMAND_MAIN_MENU);
    ASSERT_SIMPLE_COMMAND('m', vertical_unavailable_codes,
                          BOARD_COMMAND_MAIN_MENU);
    ASSERT_SIMPLE_COMMAND('M', vertical_unavailable_codes,
                          BOARD_COMMAND_MAIN_MENU);
}

static void test_board_command_from_key_shortcut_neighbours_are_ignored(void)
{
    const int keys[] = {'o', 'O', 'q', 'Q', 'l', 'L', 'y', 'Y'};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_NO_COMMAND(keys[i], test_codes);
        ASSERT_NO_COMMAND(keys[i], unavailable_codes);
    }
}

static void assert_command_keys_are_exactly(BoardCommandKind kind,
                                            const int* expected,
                                            size_t expected_count,
                                            const BoardKeyCodes* codes)
{
    for (int key = TETONOR_KEY_UNAVAILABLE; key <= TEST_SCAN_LAST_KEY; key++)
    {
        bool is_expected = false;
        for (size_t i = 0; i < expected_count; i++)
        {
            is_expected = is_expected || key == expected[i];
        }
        bool maps = board_command_from_key(key, codes).kind == kind;
        if (maps != is_expected)
        {
            TEST_HARNESS_FAIL("kind %d: key %d maps=%d expected=%d", (int)kind,
                              key, (int)maps, (int)is_expected);
        }
    }
}

static void test_board_command_from_key_each_command_has_exactly_its_keys(void)
{
    const BoardKeyCodes* code_sets[] = {&test_codes, &unavailable_codes,
                                        &vertical_unavailable_codes};
    const int escape[]               = {TETONOR_KEY_ESCAPE};
    const int pause[]                = {'p', 'P'};
    const int new_game[]             = {'n', 'N'};
    const int main_menu[]            = {'m', 'M'};
    const int undo[]                 = {'u', 'U'};
    const int theme[]                = {'t', 'T'};
    for (size_t i = 0; i < sizeof code_sets / sizeof code_sets[0]; i++)
    {
        assert_command_keys_are_exactly(BOARD_COMMAND_EXIT, escape, 1,
                                        code_sets[i]);
        assert_command_keys_are_exactly(BOARD_COMMAND_PAUSE, pause, 2,
                                        code_sets[i]);
        assert_command_keys_are_exactly(BOARD_COMMAND_NEW_GAME, new_game, 2,
                                        code_sets[i]);
        assert_command_keys_are_exactly(BOARD_COMMAND_MAIN_MENU, main_menu, 2,
                                        code_sets[i]);
        assert_command_keys_are_exactly(BOARD_COMMAND_UNDO, undo, 2,
                                        code_sets[i]);
        assert_command_keys_are_exactly(BOARD_COMMAND_THEME, theme, 2,
                                        code_sets[i]);
    }
}

static void
test_board_command_from_key_protected_keys_never_become_shortcuts(void)
{
    const int protected_keys[] = {TEST_ENTER_KEY,
                                  TEST_PAD_ENTER_KEY,
                                  TEST_LEFT_KEY,
                                  TEST_RIGHT_KEY,
                                  TEST_UP_KEY,
                                  TEST_DOWN_KEY,
                                  TEST_BACKSPACE_KEY,
                                  TEST_PAD_ZERO_KEY,
                                  TEST_PAD_PLUS_KEY,
                                  TEST_PAD_STAR_KEY,
                                  '\n',
                                  '\r',
                                  ' ',
                                  '\b',
                                  TETONOR_KEY_DELETE,
                                  '+',
                                  'x',
                                  'X',
                                  '*',
                                  '0',
                                  '9'};
    for (size_t i = 0; i < sizeof protected_keys / sizeof protected_keys[0];
         i++)
    {
        BoardCommand command =
            board_command_from_key(protected_keys[i], &test_codes);
        ASSERT_EQ_INT(command.kind, BOARD_COMMAND_INPUT);
    }
}

static void test_board_command_from_key_existing_mappings_unchanged(void)
{
    ASSERT_SIMPLE_COMMAND(TETONOR_KEY_ESCAPE, test_codes, BOARD_COMMAND_EXIT);
    ASSERT_SIMPLE_COMMAND('u', test_codes, BOARD_COMMAND_UNDO);
    ASSERT_SIMPLE_COMMAND('U', test_codes, BOARD_COMMAND_UNDO);
    for (int digit = 0; digit < TETONOR_DIGIT_BASE; digit++)
    {
        ASSERT_DIGIT_COMMAND(TETONOR_KEY_DIGIT_FIRST + digit, test_codes,
                             digit);
    }
    ASSERT_DIGIT_COMMAND(TEST_PAD_ZERO_KEY, test_codes, 0);
    ASSERT_INPUT_COMMAND(TEST_LEFT_KEY, test_codes, BOARD_INPUT_MOVE_LEFT);
    ASSERT_INPUT_COMMAND(TEST_RIGHT_KEY, test_codes, BOARD_INPUT_MOVE_RIGHT);
    ASSERT_INPUT_COMMAND(TEST_UP_KEY, test_codes, BOARD_INPUT_MOVE_UP);
    ASSERT_INPUT_COMMAND(TEST_DOWN_KEY, test_codes, BOARD_INPUT_MOVE_DOWN);
    ASSERT_OPERATION_COMMAND('+', test_codes, GROUP_OPERATION_ADD);
    ASSERT_OPERATION_COMMAND(TEST_PAD_PLUS_KEY, test_codes,
                             GROUP_OPERATION_ADD);
    ASSERT_OPERATION_COMMAND('x', test_codes, GROUP_OPERATION_MULTIPLY);
    ASSERT_OPERATION_COMMAND('X', test_codes, GROUP_OPERATION_MULTIPLY);
    ASSERT_OPERATION_COMMAND('*', test_codes, GROUP_OPERATION_MULTIPLY);
    ASSERT_OPERATION_COMMAND(TEST_PAD_STAR_KEY, test_codes,
                             GROUP_OPERATION_MULTIPLY);
    ASSERT_INPUT_COMMAND('\n', test_codes, BOARD_INPUT_ACTIVATE);
    ASSERT_INPUT_COMMAND('\r', test_codes, BOARD_INPUT_ACTIVATE);
    ASSERT_INPUT_COMMAND(' ', test_codes, BOARD_INPUT_ACTIVATE);
    ASSERT_INPUT_COMMAND(TEST_ENTER_KEY, test_codes, BOARD_INPUT_ACTIVATE);
    ASSERT_INPUT_COMMAND(TEST_PAD_ENTER_KEY, test_codes, BOARD_INPUT_ACTIVATE);
    ASSERT_INPUT_COMMAND('\b', test_codes, BOARD_INPUT_DELETE);
    ASSERT_INPUT_COMMAND(TETONOR_KEY_DELETE, test_codes, BOARD_INPUT_DELETE);
    ASSERT_INPUT_COMMAND(TEST_BACKSPACE_KEY, test_codes, BOARD_INPUT_DELETE);
}

static void test_pause_menu_command_from_key_maps_navigation(void)
{
    ASSERT_EQ_INT(pause_menu_command_from_key(TEST_UP_KEY, &menu_codes),
                  PAUSE_MENU_COMMAND_UP);
    ASSERT_EQ_INT(pause_menu_command_from_key(TEST_DOWN_KEY, &menu_codes),
                  PAUSE_MENU_COMMAND_DOWN);
    const int select_keys[] = {' ', '\n', '\r', TEST_ENTER_KEY};
    for (size_t i = 0; i < sizeof select_keys / sizeof select_keys[0]; i++)
    {
        ASSERT_EQ_INT(pause_menu_command_from_key(select_keys[i], &menu_codes),
                      PAUSE_MENU_COMMAND_SELECT);
    }
    const int resume_keys[] = {TETONOR_KEY_ESCAPE, TEST_ESCAPE_LITERAL, 'p',
                               'P'};
    for (size_t i = 0; i < sizeof resume_keys / sizeof resume_keys[0]; i++)
    {
        ASSERT_EQ_INT(pause_menu_command_from_key(resume_keys[i], &menu_codes),
                      PAUSE_MENU_COMMAND_RESUME);
    }
}

static void test_pause_menu_command_from_key_ignores_other_keys(void)
{
    const int keys[] = {'0',
                        '5',
                        '9',
                        'n',
                        'N',
                        'm',
                        'M',
                        'u',
                        'U',
                        'y',
                        'Y',
                        '+',
                        'x',
                        '*',
                        'a',
                        '\b',
                        TETONOR_KEY_DELETE,
                        TEST_BACKSPACE_KEY,
                        TEST_LEFT_KEY,
                        TEST_RIGHT_KEY,
                        TEST_HOME_KEY,
                        TEST_RESIZE_KEY,
                        0,
                        TEST_ERR_KEY,
                        TETONOR_KEY_UNAVAILABLE};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(pause_menu_command_from_key(keys[i], &menu_codes),
                      PAUSE_MENU_COMMAND_NONE);
    }
}

static void test_pause_menu_command_from_key_unavailable_codes_never_match(void)
{
    ASSERT_EQ_INT(pause_menu_command_from_key(TETONOR_KEY_UNAVAILABLE,
                                              &unavailable_menu_codes),
                  PAUSE_MENU_COMMAND_NONE);
    ASSERT_EQ_INT(
        pause_menu_command_from_key(TEST_UP_KEY, &unavailable_menu_codes),
        PAUSE_MENU_COMMAND_NONE);
    ASSERT_EQ_INT(
        pause_menu_command_from_key(TEST_DOWN_KEY, &unavailable_menu_codes),
        PAUSE_MENU_COMMAND_NONE);
    ASSERT_EQ_INT(
        pause_menu_command_from_key(TEST_ENTER_KEY, &unavailable_menu_codes),
        PAUSE_MENU_COMMAND_NONE);
    ASSERT_EQ_INT(pause_menu_command_from_key(' ', &unavailable_menu_codes),
                  PAUSE_MENU_COMMAND_SELECT);
    ASSERT_EQ_INT(pause_menu_command_from_key('P', &unavailable_menu_codes),
                  PAUSE_MENU_COMMAND_RESUME);
}

static void test_prompt_command_from_key_maps_answers(void)
{
    ASSERT_EQ_INT(prompt_command_from_key('y', &menu_codes),
                  PROMPT_COMMAND_YES);
    ASSERT_EQ_INT(prompt_command_from_key('Y', &menu_codes),
                  PROMPT_COMMAND_YES);
    ASSERT_EQ_INT(prompt_command_from_key('n', &menu_codes), PROMPT_COMMAND_NO);
    ASSERT_EQ_INT(prompt_command_from_key('N', &menu_codes), PROMPT_COMMAND_NO);
    ASSERT_EQ_INT(prompt_command_from_key(TETONOR_KEY_ESCAPE, &menu_codes),
                  PROMPT_COMMAND_NO);
    ASSERT_EQ_INT(prompt_command_from_key(TEST_UP_KEY, &menu_codes),
                  PROMPT_COMMAND_UP);
    ASSERT_EQ_INT(prompt_command_from_key(TEST_DOWN_KEY, &menu_codes),
                  PROMPT_COMMAND_DOWN);
    const int select_keys[] = {' ', '\n', '\r', TEST_ENTER_KEY};
    for (size_t i = 0; i < sizeof select_keys / sizeof select_keys[0]; i++)
    {
        ASSERT_EQ_INT(prompt_command_from_key(select_keys[i], &menu_codes),
                      PROMPT_COMMAND_SELECT);
    }
}

static void test_prompt_command_from_key_ignores_other_keys(void)
{
    const int keys[] = {'p',
                        'P',
                        'm',
                        'M',
                        'u',
                        'U',
                        '0',
                        '7',
                        'x',
                        '+',
                        'a',
                        TEST_BACKSPACE_KEY,
                        TEST_LEFT_KEY,
                        TEST_RESIZE_KEY,
                        0,
                        TEST_ERR_KEY,
                        TETONOR_KEY_UNAVAILABLE};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(prompt_command_from_key(keys[i], &menu_codes),
                      PROMPT_COMMAND_NONE);
    }
}

static void test_prompt_command_from_key_unavailable_codes_never_match(void)
{
    ASSERT_EQ_INT(prompt_command_from_key(TETONOR_KEY_UNAVAILABLE,
                                          &unavailable_menu_codes),
                  PROMPT_COMMAND_NONE);
    ASSERT_EQ_INT(prompt_command_from_key(TEST_UP_KEY, &unavailable_menu_codes),
                  PROMPT_COMMAND_NONE);
    ASSERT_EQ_INT(
        prompt_command_from_key(TEST_DOWN_KEY, &unavailable_menu_codes),
        PROMPT_COMMAND_NONE);
    ASSERT_EQ_INT(
        prompt_command_from_key(TEST_ENTER_KEY, &unavailable_menu_codes),
        PROMPT_COMMAND_NONE);
    ASSERT_EQ_INT(prompt_command_from_key('\n', &unavailable_menu_codes),
                  PROMPT_COMMAND_SELECT);
    ASSERT_EQ_INT(prompt_command_from_key('y', &unavailable_menu_codes),
                  PROMPT_COMMAND_YES);
}

static void test_menus_ignore_game_shortcuts(void)
{
    const int keys[] = {'p', 'P', 'n', 'N', 'm', 'M'};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(menu_result_from_key(keys[i], TEST_ENTER_KEY), MENU_NONE);
        ASSERT_EQ_INT(difficulty_menu_command_from_key(keys[i], &menu_codes),
                      DIFFICULTY_COMMAND_NONE);
        ASSERT_EQ_INT(stats_menu_command_from_key(keys[i]),
                      STATS_MENU_COMMAND_NONE);
    }
}

static void test_victory_menu_command_from_key_maps_navigation(void)
{
    ASSERT_EQ_INT(victory_menu_command_from_key(TEST_UP_KEY, &menu_codes),
                  VICTORY_MENU_COMMAND_UP);
    ASSERT_EQ_INT(victory_menu_command_from_key(TEST_DOWN_KEY, &menu_codes),
                  VICTORY_MENU_COMMAND_DOWN);
    const int select_keys[] = {' ', '\n', '\r', TEST_ENTER_KEY};
    for (size_t i = 0; i < sizeof select_keys / sizeof select_keys[0]; i++)
    {
        ASSERT_EQ_INT(
            victory_menu_command_from_key(select_keys[i], &menu_codes),
            VICTORY_MENU_COMMAND_SELECT);
    }
}

static void test_victory_menu_command_from_key_maps_shortcuts(void)
{
    const int new_game_keys[] = {'n', 'N'};
    for (size_t i = 0; i < sizeof new_game_keys / sizeof new_game_keys[0]; i++)
    {
        ASSERT_EQ_INT(
            victory_menu_command_from_key(new_game_keys[i], &menu_codes),
            VICTORY_MENU_COMMAND_NEW_GAME);
    }
    const int main_menu_keys[] = {'m', 'M', TETONOR_KEY_ESCAPE,
                                  TEST_ESCAPE_LITERAL};
    for (size_t i = 0; i < sizeof main_menu_keys / sizeof main_menu_keys[0];
         i++)
    {
        ASSERT_EQ_INT(
            victory_menu_command_from_key(main_menu_keys[i], &menu_codes),
            VICTORY_MENU_COMMAND_MAIN_MENU);
    }
}

static void test_victory_menu_command_from_key_ignores_other_keys(void)
{
    const int keys[] = {'p',
                        'P',
                        'u',
                        'U',
                        'y',
                        'Y',
                        'q',
                        's',
                        'S',
                        '0',
                        '5',
                        '9',
                        '+',
                        'x',
                        'X',
                        '*',
                        '\b',
                        TETONOR_KEY_DELETE,
                        TEST_BACKSPACE_KEY,
                        TEST_PAD_ENTER_KEY,
                        TEST_PAD_ZERO_KEY,
                        TEST_PAD_PLUS_KEY,
                        TEST_PAD_STAR_KEY,
                        TEST_LEFT_KEY,
                        TEST_RIGHT_KEY,
                        TEST_HOME_KEY,
                        TEST_PAGE_DOWN_KEY,
                        TEST_RESIZE_KEY,
                        0,
                        TEST_ERR_KEY,
                        TETONOR_KEY_UNAVAILABLE};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(victory_menu_command_from_key(keys[i], &menu_codes),
                      VICTORY_MENU_COMMAND_NONE);
    }
}

static void
test_victory_menu_command_from_key_unavailable_codes_never_match(void)
{
    ASSERT_EQ_INT(victory_menu_command_from_key(TETONOR_KEY_UNAVAILABLE,
                                                &unavailable_menu_codes),
                  VICTORY_MENU_COMMAND_NONE);
    ASSERT_EQ_INT(
        victory_menu_command_from_key(TEST_UP_KEY, &unavailable_menu_codes),
        VICTORY_MENU_COMMAND_NONE);
    ASSERT_EQ_INT(
        victory_menu_command_from_key(TEST_DOWN_KEY, &unavailable_menu_codes),
        VICTORY_MENU_COMMAND_NONE);
    ASSERT_EQ_INT(
        victory_menu_command_from_key(TEST_ENTER_KEY, &unavailable_menu_codes),
        VICTORY_MENU_COMMAND_NONE);
    ASSERT_EQ_INT(victory_menu_command_from_key(' ', &unavailable_menu_codes),
                  VICTORY_MENU_COMMAND_SELECT);
    ASSERT_EQ_INT(victory_menu_command_from_key('n', &unavailable_menu_codes),
                  VICTORY_MENU_COMMAND_NEW_GAME);
    ASSERT_EQ_INT(victory_menu_command_from_key('M', &unavailable_menu_codes),
                  VICTORY_MENU_COMMAND_MAIN_MENU);
}

static void test_victory_keys_leave_the_other_screens_unchanged(void)
{
    ASSERT_EQ_INT(pause_menu_command_from_key('n', &menu_codes),
                  PAUSE_MENU_COMMAND_NONE);
    ASSERT_EQ_INT(pause_menu_command_from_key('m', &menu_codes),
                  PAUSE_MENU_COMMAND_NONE);
    ASSERT_EQ_INT(pause_menu_command_from_key('p', &menu_codes),
                  PAUSE_MENU_COMMAND_RESUME);
    ASSERT_EQ_INT(prompt_command_from_key('n', &menu_codes), PROMPT_COMMAND_NO);
    ASSERT_EQ_INT(prompt_command_from_key('m', &menu_codes),
                  PROMPT_COMMAND_NONE);
    ASSERT_EQ_INT(board_command_from_key('p', &test_codes).kind,
                  BOARD_COMMAND_PAUSE);
    ASSERT_EQ_INT(board_command_from_key('n', &test_codes).kind,
                  BOARD_COMMAND_NEW_GAME);
    ASSERT_EQ_INT(board_command_from_key('M', &test_codes).kind,
                  BOARD_COMMAND_MAIN_MENU);
    ASSERT_EQ_INT(
        difficulty_menu_command_from_key(TETONOR_KEY_ESCAPE, &menu_codes),
        DIFFICULTY_COMMAND_BACK);
    ASSERT_EQ_INT(stats_menu_command_from_key(TETONOR_KEY_ESCAPE),
                  STATS_MENU_COMMAND_BACK);
}

static void test_theme_key_codes_are_t(void)
{
    ASSERT_EQ_INT(TETONOR_KEY_THEME, 't');
    ASSERT_EQ_INT(TETONOR_KEY_THEME_UPPER, 'T');
}

static void test_menu_result_from_key_t_opens_theme(void)
{
    ASSERT_EQ_INT(menu_result_from_key('t', TEST_ENTER_KEY), MENU_THEME);
    ASSERT_EQ_INT(menu_result_from_key('T', TEST_ENTER_KEY), MENU_THEME);
    ASSERT_EQ_INT(menu_result_from_key(TETONOR_KEY_THEME, TEST_ENTER_KEY),
                  MENU_THEME);
    ASSERT_EQ_INT(menu_result_from_key(TETONOR_KEY_THEME_UPPER, TEST_ENTER_KEY),
                  MENU_THEME);
}

static void test_menu_result_from_key_t_keeps_the_other_mappings(void)
{
    ASSERT_EQ_INT(menu_result_from_key('s', TEST_ENTER_KEY), MENU_STATISTICS);
    ASSERT_EQ_INT(menu_result_from_key(' ', TEST_ENTER_KEY), MENU_START);
    ASSERT_EQ_INT(menu_result_from_key(TETONOR_KEY_ESCAPE, TEST_ENTER_KEY),
                  MENU_EXIT);
    ASSERT_EQ_INT(menu_result_from_key('v', TEST_ENTER_KEY), MENU_NONE);
}

static void test_board_command_from_key_t_opens_theme(void)
{
    ASSERT_SIMPLE_COMMAND('t', test_codes, BOARD_COMMAND_THEME);
    ASSERT_SIMPLE_COMMAND('T', test_codes, BOARD_COMMAND_THEME);
    ASSERT_SIMPLE_COMMAND('t', unavailable_codes, BOARD_COMMAND_THEME);
    ASSERT_SIMPLE_COMMAND('T', unavailable_codes, BOARD_COMMAND_THEME);
    ASSERT_SIMPLE_COMMAND('t', vertical_unavailable_codes, BOARD_COMMAND_THEME);
    ASSERT_SIMPLE_COMMAND('T', vertical_unavailable_codes, BOARD_COMMAND_THEME);
}

static void test_board_command_from_key_theme_keeps_existing_mappings(void)
{
    ASSERT_SIMPLE_COMMAND('p', test_codes, BOARD_COMMAND_PAUSE);
    ASSERT_SIMPLE_COMMAND('u', test_codes, BOARD_COMMAND_UNDO);
    ASSERT_SIMPLE_COMMAND('n', test_codes, BOARD_COMMAND_NEW_GAME);
    ASSERT_SIMPLE_COMMAND('m', test_codes, BOARD_COMMAND_MAIN_MENU);
    ASSERT_SIMPLE_COMMAND(TETONOR_KEY_ESCAPE, test_codes, BOARD_COMMAND_EXIT);
    ASSERT_NO_COMMAND('v', test_codes);
    ASSERT_NO_COMMAND('V', test_codes);
}

static void test_pause_menu_command_from_key_t_opens_theme(void)
{
    ASSERT_EQ_INT(pause_menu_command_from_key('t', &menu_codes),
                  PAUSE_MENU_COMMAND_THEME);
    ASSERT_EQ_INT(pause_menu_command_from_key('T', &menu_codes),
                  PAUSE_MENU_COMMAND_THEME);
    ASSERT_EQ_INT(pause_menu_command_from_key('t', &unavailable_menu_codes),
                  PAUSE_MENU_COMMAND_THEME);
    ASSERT_EQ_INT(pause_menu_command_from_key('T', &unavailable_menu_codes),
                  PAUSE_MENU_COMMAND_THEME);
}

static void test_pause_menu_command_from_key_theme_keeps_resume_keys(void)
{
    const int resume_keys[] = {TETONOR_KEY_ESCAPE, 'p', 'P'};
    for (size_t i = 0; i < sizeof resume_keys / sizeof resume_keys[0]; i++)
    {
        ASSERT_EQ_INT(pause_menu_command_from_key(resume_keys[i], &menu_codes),
                      PAUSE_MENU_COMMAND_RESUME);
    }
}

static void test_theme_menu_command_from_key_maps_navigation(void)
{
    ASSERT_EQ_INT(theme_menu_command_from_key(TEST_UP_KEY, &menu_codes),
                  THEME_MENU_COMMAND_UP);
    ASSERT_EQ_INT(theme_menu_command_from_key(TEST_DOWN_KEY, &menu_codes),
                  THEME_MENU_COMMAND_DOWN);
    const int select_keys[] = {' ', '\n', '\r', TEST_ENTER_KEY};
    for (size_t i = 0; i < sizeof select_keys / sizeof select_keys[0]; i++)
    {
        ASSERT_EQ_INT(theme_menu_command_from_key(select_keys[i], &menu_codes),
                      THEME_MENU_COMMAND_SELECT);
    }
}

static void test_theme_menu_command_from_key_maps_back(void)
{
    const int back_keys[] = {TETONOR_KEY_ESCAPE, TEST_ESCAPE_LITERAL, 't', 'T'};
    for (size_t i = 0; i < sizeof back_keys / sizeof back_keys[0]; i++)
    {
        ASSERT_EQ_INT(theme_menu_command_from_key(back_keys[i], &menu_codes),
                      THEME_MENU_COMMAND_BACK);
        ASSERT_EQ_INT(
            theme_menu_command_from_key(back_keys[i], &unavailable_menu_codes),
            THEME_MENU_COMMAND_BACK);
    }
}

static void test_theme_menu_command_from_key_ignores_other_keys(void)
{
    const int keys[] = {'0',
                        '5',
                        '9',
                        'p',
                        'P',
                        'u',
                        'U',
                        'n',
                        'N',
                        'm',
                        'M',
                        's',
                        'S',
                        'y',
                        'Y',
                        '+',
                        'x',
                        'X',
                        '*',
                        'a',
                        '\b',
                        TETONOR_KEY_DELETE,
                        TEST_BACKSPACE_KEY,
                        TEST_PAD_ENTER_KEY,
                        TEST_LEFT_KEY,
                        TEST_RIGHT_KEY,
                        TEST_HOME_KEY,
                        TEST_RESIZE_KEY,
                        0,
                        TEST_ERR_KEY,
                        TETONOR_KEY_UNAVAILABLE};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(theme_menu_command_from_key(keys[i], &menu_codes),
                      THEME_MENU_COMMAND_NONE);
    }
}

static void test_theme_menu_command_from_key_unavailable_codes_never_match(void)
{
    ASSERT_EQ_INT(theme_menu_command_from_key(TETONOR_KEY_UNAVAILABLE,
                                              &unavailable_menu_codes),
                  THEME_MENU_COMMAND_NONE);
    ASSERT_EQ_INT(
        theme_menu_command_from_key(TEST_UP_KEY, &unavailable_menu_codes),
        THEME_MENU_COMMAND_NONE);
    ASSERT_EQ_INT(
        theme_menu_command_from_key(TEST_DOWN_KEY, &unavailable_menu_codes),
        THEME_MENU_COMMAND_NONE);
    ASSERT_EQ_INT(
        theme_menu_command_from_key(TEST_ENTER_KEY, &unavailable_menu_codes),
        THEME_MENU_COMMAND_NONE);
    ASSERT_EQ_INT(theme_menu_command_from_key(' ', &unavailable_menu_codes),
                  THEME_MENU_COMMAND_SELECT);
    ASSERT_EQ_INT(theme_menu_command_from_key('\n', &unavailable_menu_codes),
                  THEME_MENU_COMMAND_SELECT);
}

static void test_prompt_victory_and_other_screens_ignore_t(void)
{
    const int keys[] = {'t', 'T'};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(prompt_command_from_key(keys[i], &menu_codes),
                      PROMPT_COMMAND_NONE);
        ASSERT_EQ_INT(victory_menu_command_from_key(keys[i], &menu_codes),
                      VICTORY_MENU_COMMAND_NONE);
        ASSERT_EQ_INT(difficulty_menu_command_from_key(keys[i], &menu_codes),
                      DIFFICULTY_COMMAND_NONE);
        ASSERT_EQ_INT(stats_menu_command_from_key(keys[i]),
                      STATS_MENU_COMMAND_NONE);
    }
}

static void test_save_and_continue_key_constants(void)
{
    ASSERT_EQ_INT(TETONOR_KEY_SAVE, 's');
    ASSERT_EQ_INT(TETONOR_KEY_SAVE_UPPER, 'S');
    ASSERT_EQ_INT(TETONOR_KEY_CONTINUE, 'c');
    ASSERT_EQ_INT(TETONOR_KEY_CONTINUE_UPPER, 'C');
}

static void test_pause_menu_command_from_key_maps_the_save_keys(void)
{
    ASSERT_EQ_INT(pause_menu_command_from_key('s', &menu_codes),
                  PAUSE_MENU_COMMAND_SAVE);
    ASSERT_EQ_INT(pause_menu_command_from_key('S', &menu_codes),
                  PAUSE_MENU_COMMAND_SAVE);
    ASSERT_EQ_INT(pause_menu_command_from_key(TETONOR_KEY_SAVE, &menu_codes),
                  PAUSE_MENU_COMMAND_SAVE);
    ASSERT_EQ_INT(
        pause_menu_command_from_key(TETONOR_KEY_SAVE_UPPER, &menu_codes),
        PAUSE_MENU_COMMAND_SAVE);
}

static void test_pause_menu_command_from_key_save_needs_no_curses_codes(void)
{
    ASSERT_EQ_INT(pause_menu_command_from_key('s', &unavailable_menu_codes),
                  PAUSE_MENU_COMMAND_SAVE);
    ASSERT_EQ_INT(pause_menu_command_from_key('S', &unavailable_menu_codes),
                  PAUSE_MENU_COMMAND_SAVE);
}

static void
test_pause_menu_command_from_key_neighbours_of_save_are_ignored(void)
{
    const int keys[] = {'a', 'A', 'd', 'D', 'w', 'W', 'x', 'X', 'z', 'c', 'C'};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(pause_menu_command_from_key(keys[i], &menu_codes),
                      PAUSE_MENU_COMMAND_NONE);
    }
}

static void test_pause_menu_command_from_key_keeps_the_other_mappings(void)
{
    ASSERT_EQ_INT(pause_menu_command_from_key(TEST_UP_KEY, &menu_codes),
                  PAUSE_MENU_COMMAND_UP);
    ASSERT_EQ_INT(pause_menu_command_from_key(TEST_DOWN_KEY, &menu_codes),
                  PAUSE_MENU_COMMAND_DOWN);
    ASSERT_EQ_INT(pause_menu_command_from_key(' ', &menu_codes),
                  PAUSE_MENU_COMMAND_SELECT);
    ASSERT_EQ_INT(pause_menu_command_from_key('p', &menu_codes),
                  PAUSE_MENU_COMMAND_RESUME);
    ASSERT_EQ_INT(pause_menu_command_from_key(TETONOR_KEY_ESCAPE, &menu_codes),
                  PAUSE_MENU_COMMAND_RESUME);
}

static void test_menu_result_from_key_c_continues(void)
{
    ASSERT_EQ_INT(menu_result_from_key('c', TEST_ENTER_KEY), MENU_CONTINUE);
    ASSERT_EQ_INT(menu_result_from_key('C', TEST_ENTER_KEY), MENU_CONTINUE);
    ASSERT_EQ_INT(menu_result_from_key(TETONOR_KEY_CONTINUE, TEST_ENTER_KEY),
                  MENU_CONTINUE);
    ASSERT_EQ_INT(
        menu_result_from_key(TETONOR_KEY_CONTINUE_UPPER, TEST_ENTER_KEY),
        MENU_CONTINUE);
}

static void test_menu_result_from_key_continue_is_its_own_result(void)
{
    ASSERT_TRUE(MENU_CONTINUE != MENU_START);
    ASSERT_TRUE(MENU_CONTINUE != MENU_EXIT);
    ASSERT_TRUE(MENU_CONTINUE != MENU_NONE);
    ASSERT_TRUE(MENU_CONTINUE != MENU_STATISTICS);
    ASSERT_EQ_INT(MENU_START, 0);
    ASSERT_EQ_INT(MENU_EXIT, 1);
    ASSERT_EQ_INT(MENU_NONE, 2);
    ASSERT_EQ_INT(MENU_STATISTICS, 3);
}

static void test_menu_result_from_key_keeps_the_other_mappings(void)
{
    ASSERT_EQ_INT(menu_result_from_key(' ', TEST_ENTER_KEY), MENU_START);
    ASSERT_EQ_INT(menu_result_from_key('\n', TEST_ENTER_KEY), MENU_START);
    ASSERT_EQ_INT(menu_result_from_key('\r', TEST_ENTER_KEY), MENU_START);
    ASSERT_EQ_INT(menu_result_from_key(TEST_ENTER_KEY, TEST_ENTER_KEY),
                  MENU_START);
    ASSERT_EQ_INT(menu_result_from_key(TETONOR_KEY_ESCAPE, TEST_ENTER_KEY),
                  MENU_EXIT);
    ASSERT_EQ_INT(menu_result_from_key('s', TEST_ENTER_KEY), MENU_STATISTICS);
    ASSERT_EQ_INT(menu_result_from_key('S', TEST_ENTER_KEY), MENU_STATISTICS);
}

static void test_menu_result_from_key_neighbours_of_c_are_ignored(void)
{
    const int keys[] = {'b', 'B', 'v', 'V', 'x', 'X', 'd', 'D', 'f', 'F'};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(menu_result_from_key(keys[i], TEST_ENTER_KEY), MENU_NONE);
    }
}

static void test_the_continue_key_is_ignored_on_every_other_screen(void)
{
    const int keys[] = {'c', 'C'};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(board_command_from_key(keys[i], &test_codes).kind,
                      BOARD_COMMAND_NONE);
        ASSERT_EQ_INT(pause_menu_command_from_key(keys[i], &menu_codes),
                      PAUSE_MENU_COMMAND_NONE);
        ASSERT_EQ_INT(prompt_command_from_key(keys[i], &menu_codes),
                      PROMPT_COMMAND_NONE);
        ASSERT_EQ_INT(difficulty_menu_command_from_key(keys[i], &menu_codes),
                      DIFFICULTY_COMMAND_NONE);
        ASSERT_EQ_INT(victory_menu_command_from_key(keys[i], &menu_codes),
                      VICTORY_MENU_COMMAND_NONE);
        ASSERT_EQ_INT(stats_menu_command_from_key(keys[i]),
                      STATS_MENU_COMMAND_NONE);
    }
}

static void test_the_save_key_is_ignored_outside_the_pause_menu(void)
{
    const int keys[] = {'s', 'S'};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(board_command_from_key(keys[i], &test_codes).kind,
                      BOARD_COMMAND_NONE);
        ASSERT_EQ_INT(prompt_command_from_key(keys[i], &menu_codes),
                      PROMPT_COMMAND_NONE);
        ASSERT_EQ_INT(difficulty_menu_command_from_key(keys[i], &menu_codes),
                      DIFFICULTY_COMMAND_NONE);
        ASSERT_EQ_INT(victory_menu_command_from_key(keys[i], &menu_codes),
                      VICTORY_MENU_COMMAND_NONE);
        ASSERT_EQ_INT(stats_menu_command_from_key(keys[i]),
                      STATS_MENU_COMMAND_NONE);
    }
}

static void test_board_command_from_key_hint_key_codes_are_h(void)
{
    ASSERT_EQ_INT(TETONOR_KEY_HINT, 'h');
    ASSERT_EQ_INT(TETONOR_KEY_HINT_UPPER, 'H');
}

static void test_board_command_from_key_h_shows_a_hint(void)
{
    ASSERT_SIMPLE_COMMAND('h', test_codes, BOARD_COMMAND_HINT);
    ASSERT_SIMPLE_COMMAND('H', test_codes, BOARD_COMMAND_HINT);
    ASSERT_SIMPLE_COMMAND('h', unavailable_codes, BOARD_COMMAND_HINT);
    ASSERT_SIMPLE_COMMAND('H', unavailable_codes, BOARD_COMMAND_HINT);
    ASSERT_SIMPLE_COMMAND('h', vertical_unavailable_codes, BOARD_COMMAND_HINT);
    ASSERT_SIMPLE_COMMAND('H', vertical_unavailable_codes, BOARD_COMMAND_HINT);
}

static void test_board_command_from_key_hint_neighbours_are_ignored(void)
{
    const int keys[] = {'g', 'G', 'i', 'I'};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_NO_COMMAND(keys[i], test_codes);
        ASSERT_NO_COMMAND(keys[i], unavailable_codes);
    }
}

static void test_board_command_from_key_hint_has_exactly_its_keys(void)
{
    const BoardKeyCodes* code_sets[] = {&test_codes, &unavailable_codes,
                                        &vertical_unavailable_codes};
    const int hint[]                 = {'h', 'H'};
    for (size_t i = 0; i < sizeof code_sets / sizeof code_sets[0]; i++)
    {
        assert_command_keys_are_exactly(BOARD_COMMAND_HINT, hint, 2,
                                        code_sets[i]);
    }
}

static void test_board_command_from_key_hint_keeps_existing_mappings(void)
{
    ASSERT_SIMPLE_COMMAND('u', test_codes, BOARD_COMMAND_UNDO);
    ASSERT_SIMPLE_COMMAND('p', test_codes, BOARD_COMMAND_PAUSE);
    ASSERT_SIMPLE_COMMAND('n', test_codes, BOARD_COMMAND_NEW_GAME);
    ASSERT_SIMPLE_COMMAND('m', test_codes, BOARD_COMMAND_MAIN_MENU);
    ASSERT_SIMPLE_COMMAND('t', test_codes, BOARD_COMMAND_THEME);
    ASSERT_SIMPLE_COMMAND(TETONOR_KEY_ESCAPE, test_codes, BOARD_COMMAND_EXIT);
    ASSERT_INPUT_COMMAND(TEST_ENTER_KEY, test_codes, BOARD_INPUT_ACTIVATE);
    ASSERT_DIGIT_COMMAND('5', test_codes, 5);
}

static void test_the_hint_key_is_ignored_on_every_other_screen(void)
{
    const int keys[] = {'h', 'H'};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(menu_result_from_key(keys[i], TEST_ENTER_KEY), MENU_NONE);
        ASSERT_EQ_INT(pause_menu_command_from_key(keys[i], &menu_codes),
                      PAUSE_MENU_COMMAND_NONE);
        ASSERT_EQ_INT(prompt_command_from_key(keys[i], &menu_codes),
                      PROMPT_COMMAND_NONE);
        ASSERT_EQ_INT(difficulty_menu_command_from_key(keys[i], &menu_codes),
                      DIFFICULTY_COMMAND_NONE);
        ASSERT_EQ_INT(victory_menu_command_from_key(keys[i], &menu_codes),
                      VICTORY_MENU_COMMAND_NONE);
        ASSERT_EQ_INT(theme_menu_command_from_key(keys[i], &menu_codes),
                      THEME_MENU_COMMAND_NONE);
        ASSERT_EQ_INT(stats_menu_command_from_key(keys[i]),
                      STATS_MENU_COMMAND_NONE);
    }
}

int main(void)
{
    RUN_TEST(test_menu_result_from_key_space_starts);
    RUN_TEST(test_menu_result_from_key_newline_starts);
    RUN_TEST(test_menu_result_from_key_carriage_return_starts);
    RUN_TEST(test_menu_result_from_key_enter_key_starts);
    RUN_TEST(test_menu_result_from_key_escape_exits);
    RUN_TEST(test_menu_result_from_key_unknown_key_is_ignored);
    RUN_TEST(test_board_command_from_key_escape_exits);
    RUN_TEST(test_board_command_from_key_arrows_move_cursor);
    RUN_TEST(test_board_command_from_key_main_row_digits_type_digit);
    RUN_TEST(test_board_command_from_key_every_digit_maps_to_its_value);
    RUN_TEST(test_board_command_from_key_numpad_zero_types_zero);
    RUN_TEST(test_board_command_from_key_arrows_move_focus_vertically);
    RUN_TEST(test_board_command_from_key_enter_variants_activate);
    RUN_TEST(test_board_command_from_key_backspace_variants_delete);
    RUN_TEST(test_board_command_from_key_space_activates);
    RUN_TEST(test_board_command_from_key_plus_adds);
    RUN_TEST(test_board_command_from_key_keypad_plus_adds);
    RUN_TEST(test_board_command_from_key_multiplication_keys_multiply);
    RUN_TEST(test_board_command_from_key_keypad_star_multiplies);
    RUN_TEST(test_board_command_from_key_operation_characters_ignore_codes);
    RUN_TEST(test_board_command_from_key_other_operators_are_ignored);
    RUN_TEST(test_board_command_from_key_digit_range_neighbours_are_ignored);
    RUN_TEST(test_board_command_from_key_unrelated_keys_are_ignored);
    RUN_TEST(test_board_command_from_key_unavailable_codes_never_match);
    RUN_TEST(
        test_board_command_from_key_unavailable_vertical_codes_never_match);
    RUN_TEST(test_board_command_from_key_available_codes_still_work);
    RUN_TEST(test_difficulty_menu_command_from_key_arrows_move);
    RUN_TEST(test_difficulty_menu_command_from_key_enter_variants_confirm);
    RUN_TEST(test_difficulty_menu_command_from_key_escape_goes_back);
    RUN_TEST(test_difficulty_menu_command_from_key_unrelated_keys_are_ignored);
    RUN_TEST(
        test_difficulty_menu_command_from_key_unavailable_codes_never_match);
    RUN_TEST(test_difficulty_menu_keys_select_hard_then_confirm);
    RUN_TEST(test_difficulty_menu_keys_escape_returns_without_starting);
    RUN_TEST(test_difficulty_menu_keys_enter_on_fresh_menu_starts_medium);
    RUN_TEST(test_board_command_from_key_undo_key_codes_are_u);
    RUN_TEST(test_board_command_from_key_u_undoes);
    RUN_TEST(test_board_command_from_key_undo_neighbours_are_ignored);
    RUN_TEST(test_board_command_from_key_undo_keeps_existing_mappings);
    RUN_TEST(test_menus_do_not_react_to_the_undo_key);
    RUN_TEST(test_difficulty_menu_command_from_key_ignores_the_undo_key);
    RUN_TEST(test_menu_result_from_key_s_opens_statistics);
    RUN_TEST(test_menu_result_from_key_existing_mappings_unchanged);
    RUN_TEST(test_menu_result_from_key_neighbours_of_s_are_ignored);
    RUN_TEST(test_board_command_from_key_s_is_ignored_in_game);
    RUN_TEST(test_difficulty_menu_command_from_key_ignores_the_s_key);
    RUN_TEST(test_stats_menu_command_from_key);
    RUN_TEST(test_board_command_from_key_p_pauses);
    RUN_TEST(test_board_command_from_key_n_starts_new_game);
    RUN_TEST(test_board_command_from_key_m_returns_to_main_menu);
    RUN_TEST(test_board_command_from_key_shortcut_neighbours_are_ignored);
    RUN_TEST(test_board_command_from_key_each_command_has_exactly_its_keys);
    RUN_TEST(test_board_command_from_key_protected_keys_never_become_shortcuts);
    RUN_TEST(test_board_command_from_key_existing_mappings_unchanged);
    RUN_TEST(test_pause_menu_command_from_key_maps_navigation);
    RUN_TEST(test_pause_menu_command_from_key_ignores_other_keys);
    RUN_TEST(test_pause_menu_command_from_key_unavailable_codes_never_match);
    RUN_TEST(test_prompt_command_from_key_maps_answers);
    RUN_TEST(test_prompt_command_from_key_ignores_other_keys);
    RUN_TEST(test_prompt_command_from_key_unavailable_codes_never_match);
    RUN_TEST(test_menus_ignore_game_shortcuts);
    RUN_TEST(test_victory_menu_command_from_key_maps_navigation);
    RUN_TEST(test_victory_menu_command_from_key_maps_shortcuts);
    RUN_TEST(test_victory_menu_command_from_key_ignores_other_keys);
    RUN_TEST(test_victory_menu_command_from_key_unavailable_codes_never_match);
    RUN_TEST(test_victory_keys_leave_the_other_screens_unchanged);
    RUN_TEST(test_theme_key_codes_are_t);
    RUN_TEST(test_menu_result_from_key_t_opens_theme);
    RUN_TEST(test_menu_result_from_key_t_keeps_the_other_mappings);
    RUN_TEST(test_board_command_from_key_t_opens_theme);
    RUN_TEST(test_board_command_from_key_theme_keeps_existing_mappings);
    RUN_TEST(test_pause_menu_command_from_key_t_opens_theme);
    RUN_TEST(test_pause_menu_command_from_key_theme_keeps_resume_keys);
    RUN_TEST(test_theme_menu_command_from_key_maps_navigation);
    RUN_TEST(test_theme_menu_command_from_key_maps_back);
    RUN_TEST(test_theme_menu_command_from_key_ignores_other_keys);
    RUN_TEST(test_theme_menu_command_from_key_unavailable_codes_never_match);
    RUN_TEST(test_prompt_victory_and_other_screens_ignore_t);
    RUN_TEST(test_save_and_continue_key_constants);
    RUN_TEST(test_pause_menu_command_from_key_maps_the_save_keys);
    RUN_TEST(test_pause_menu_command_from_key_save_needs_no_curses_codes);
    RUN_TEST(test_pause_menu_command_from_key_neighbours_of_save_are_ignored);
    RUN_TEST(test_pause_menu_command_from_key_keeps_the_other_mappings);
    RUN_TEST(test_menu_result_from_key_c_continues);
    RUN_TEST(test_menu_result_from_key_continue_is_its_own_result);
    RUN_TEST(test_menu_result_from_key_keeps_the_other_mappings);
    RUN_TEST(test_menu_result_from_key_neighbours_of_c_are_ignored);
    RUN_TEST(test_the_continue_key_is_ignored_on_every_other_screen);
    RUN_TEST(test_the_save_key_is_ignored_outside_the_pause_menu);
    RUN_TEST(test_board_command_from_key_hint_key_codes_are_h);
    RUN_TEST(test_board_command_from_key_h_shows_a_hint);
    RUN_TEST(test_board_command_from_key_hint_neighbours_are_ignored);
    RUN_TEST(test_board_command_from_key_hint_has_exactly_its_keys);
    RUN_TEST(test_board_command_from_key_hint_keeps_existing_mappings);
    RUN_TEST(test_the_hint_key_is_ignored_on_every_other_screen);
    return test_harness_result();
}
