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
#define TEST_HOME_KEY 0x106
#define TEST_PAGE_DOWN_KEY 0x152
#define TEST_RESIZE_KEY 0x19a
#define TEST_ERR_KEY (-1)

static const BoardKeyCodes test_codes = {
    TEST_ENTER_KEY,     TEST_PAD_ENTER_KEY, TEST_LEFT_KEY, TEST_RIGHT_KEY,
    TEST_BACKSPACE_KEY, TEST_PAD_ZERO_KEY,  TEST_UP_KEY,   TEST_DOWN_KEY};

static const BoardKeyCodes unavailable_codes = {
    TEST_ENTER_KEY,     TETONOR_KEY_UNAVAILABLE, TEST_LEFT_KEY, TEST_RIGHT_KEY,
    TEST_BACKSPACE_KEY, TETONOR_KEY_UNAVAILABLE, TEST_UP_KEY,   TEST_DOWN_KEY};

static const BoardKeyCodes vertical_unavailable_codes = {
    TEST_ENTER_KEY,          TEST_PAD_ENTER_KEY,     TEST_LEFT_KEY,
    TEST_RIGHT_KEY,          TEST_BACKSPACE_KEY,     TEST_PAD_ZERO_KEY,
    TETONOR_KEY_UNAVAILABLE, TETONOR_KEY_UNAVAILABLE};

#define ASSERT_INPUT_COMMAND(key, codes, expected_kind)                        \
    do                                                                         \
    {                                                                          \
        BoardCommand command_under_test =                                      \
            board_command_from_key((key), &(codes));                           \
        ASSERT_EQ_INT(command_under_test.kind, BOARD_COMMAND_INPUT);           \
        ASSERT_EQ_INT(command_under_test.input.kind, (expected_kind));         \
    } while (0)

#define ASSERT_DIGIT_COMMAND(key, codes, expected_digit)                       \
    do                                                                         \
    {                                                                          \
        BoardCommand command_under_test =                                      \
            board_command_from_key((key), &(codes));                           \
        ASSERT_EQ_INT(command_under_test.kind, BOARD_COMMAND_INPUT);           \
        ASSERT_EQ_INT(command_under_test.input.kind, BOARD_INPUT_DIGIT);       \
        ASSERT_EQ_INT(command_under_test.input.digit, (expected_digit));       \
    } while (0)

#define ASSERT_NO_COMMAND(key, codes)                                          \
    do                                                                         \
    {                                                                          \
        BoardCommand command_under_test =                                      \
            board_command_from_key((key), &(codes));                           \
        ASSERT_EQ_INT(command_under_test.kind, BOARD_COMMAND_NONE);            \
        ASSERT_EQ_INT(command_under_test.input.kind, 0);                       \
        ASSERT_EQ_INT(command_under_test.input.digit, 0);                      \
    } while (0)

static void test_board_command_from_key_escape_exits(void)
{
    BoardCommand command =
        board_command_from_key(TETONOR_KEY_ESCAPE, &test_codes);
    ASSERT_EQ_INT(command.kind, BOARD_COMMAND_EXIT);
    ASSERT_EQ_INT(command.input.kind, 0);
    ASSERT_EQ_INT(command.input.digit, 0);
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
    RUN_TEST(test_board_command_from_key_digit_range_neighbours_are_ignored);
    RUN_TEST(test_board_command_from_key_unrelated_keys_are_ignored);
    RUN_TEST(test_board_command_from_key_unavailable_codes_never_match);
    RUN_TEST(
        test_board_command_from_key_unavailable_vertical_codes_never_match);
    RUN_TEST(test_board_command_from_key_available_codes_still_work);
    return test_harness_result();
}
