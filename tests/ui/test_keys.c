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

int main(void)
{
    RUN_TEST(test_menu_result_from_key_space_starts);
    RUN_TEST(test_menu_result_from_key_newline_starts);
    RUN_TEST(test_menu_result_from_key_carriage_return_starts);
    RUN_TEST(test_menu_result_from_key_enter_key_starts);
    RUN_TEST(test_menu_result_from_key_escape_exits);
    RUN_TEST(test_menu_result_from_key_unknown_key_is_ignored);
    return test_harness_result();
}
