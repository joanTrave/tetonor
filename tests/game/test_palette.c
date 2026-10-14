#include <stdbool.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/game/palette.h"

#define TEST_ROLE_COUNT 7
#define TEST_BASIC_COLOR_COUNT 8
#define TEST_GARBAGE_BYTE 0xA5

static bool entry_is(const PaletteEntry* entry, PaletteColor color,
                     PaletteColor fallback)
{
    return entry->color == color && entry->fallback == fallback;
}

static bool entries_equal(const PaletteEntry* first, const PaletteEntry* second)
{
    return entry_is(first, second->color, second->fallback);
}

static bool palettes_equal(const Palette* first, const Palette* second)
{
    for (int role = 0; role < PALETTE_ROLE_COUNT; role++)
    {
        if (!entries_equal(&first->foregrounds[role],
                           &second->foregrounds[role]))
        {
            return false;
        }
    }
    return entries_equal(&first->background, &second->background);
}

static void test_palette_has_the_planned_roles(void)
{
    ASSERT_EQ_INT(PALETTE_ROLE_COUNT, TEST_ROLE_COUNT);
}

static void test_palette_colors_are_appended_in_order(void)
{
    ASSERT_EQ_INT(PALETTE_COLOR_BLACK, 0);
    ASSERT_EQ_INT(PALETTE_COLOR_WHITE, TEST_BASIC_COLOR_COUNT - 1);
    ASSERT_EQ_INT(PALETTE_COLOR_DARK_GREY, TEST_BASIC_COLOR_COUNT);
    ASSERT_EQ_INT(PALETTE_COLOR_BRIGHT_BLUE, PALETTE_COLOR_DARK_GREY + 1);
    ASSERT_EQ_INT(PALETTE_COLOR_BRIGHT_GREEN, PALETTE_COLOR_BRIGHT_BLUE + 1);
}

static void test_init_palette_entered_is_dark_grey_with_yellow_fallback(void)
{
    Palette palette;
    init_palette(&palette);
    ASSERT_TRUE(entry_is(&palette.foregrounds[PALETTE_ROLE_ENTERED],
                         PALETTE_COLOR_DARK_GREY, PALETTE_COLOR_YELLOW));
}

static void test_init_palette_correct_is_green(void)
{
    Palette palette;
    init_palette(&palette);
    ASSERT_TRUE(entry_is(&palette.foregrounds[PALETTE_ROLE_CORRECT],
                         PALETTE_COLOR_GREEN, PALETTE_COLOR_GREEN));
}

static void test_init_palette_wrong_is_red(void)
{
    Palette palette;
    init_palette(&palette);
    ASSERT_TRUE(entry_is(&palette.foregrounds[PALETTE_ROLE_WRONG],
                         PALETTE_COLOR_RED, PALETTE_COLOR_RED));
}

static void test_init_palette_used_once_is_blue(void)
{
    Palette palette;
    init_palette(&palette);
    ASSERT_TRUE(entry_is(&palette.foregrounds[PALETTE_ROLE_USED_ONCE],
                         PALETTE_COLOR_BLUE, PALETTE_COLOR_BLUE));
}

static void test_init_palette_used_twice_is_green(void)
{
    Palette palette;
    init_palette(&palette);
    ASSERT_TRUE(entry_is(&palette.foregrounds[PALETTE_ROLE_USED_TWICE],
                         PALETTE_COLOR_GREEN, PALETTE_COLOR_GREEN));
}

static void test_init_palette_used_once_typed_is_bright_blue(void)
{
    Palette palette;
    init_palette(&palette);
    ASSERT_TRUE(entry_is(&palette.foregrounds[PALETTE_ROLE_USED_ONCE_TYPED],
                         PALETTE_COLOR_BRIGHT_BLUE, PALETTE_COLOR_BLUE));
}

static void test_init_palette_used_twice_typed_is_bright_green(void)
{
    Palette palette;
    init_palette(&palette);
    ASSERT_TRUE(entry_is(&palette.foregrounds[PALETTE_ROLE_USED_TWICE_TYPED],
                         PALETTE_COLOR_BRIGHT_GREEN, PALETTE_COLOR_GREEN));
}

static void test_init_palette_background_is_black(void)
{
    Palette palette;
    init_palette(&palette);
    ASSERT_TRUE(entry_is(&palette.background, PALETTE_COLOR_BLACK,
                         PALETTE_COLOR_BLACK));
}

static void test_init_palette_replaces_previous_contents(void)
{
    Palette expected;
    init_palette(&expected);
    Palette palette;
    memset(&palette, TEST_GARBAGE_BYTE, sizeof palette);
    init_palette(&palette);
    ASSERT_TRUE(palettes_equal(&palette, &expected));
}

static void test_init_palette_is_repeatable(void)
{
    Palette first;
    Palette second;
    init_palette(&first);
    init_palette(&second);
    init_palette(&second);
    ASSERT_TRUE(palettes_equal(&first, &second));
}

static void test_init_palette_after_edit_restores_defaults(void)
{
    Palette expected;
    init_palette(&expected);
    Palette palette                                   = expected;
    palette.foregrounds[PALETTE_ROLE_USED_ONCE].color = PALETTE_COLOR_MAGENTA;
    palette.background.fallback                       = PALETTE_COLOR_WHITE;
    ASSERT_FALSE(palettes_equal(&palette, &expected));
    init_palette(&palette);
    ASSERT_TRUE(palettes_equal(&palette, &expected));
}

static void test_palette_copy_is_an_independent_value(void)
{
    Palette original;
    init_palette(&original);
    Palette copy = original;
    ASSERT_TRUE(palettes_equal(&copy, &original));
    copy.foregrounds[PALETTE_ROLE_ENTERED].color = PALETTE_COLOR_WHITE;
    ASSERT_FALSE(palettes_equal(&copy, &original));
    ASSERT_EQ_INT(original.foregrounds[PALETTE_ROLE_ENTERED].color,
                  PALETTE_COLOR_DARK_GREY);
}

int main(void)
{
    RUN_TEST(test_palette_has_the_planned_roles);
    RUN_TEST(test_palette_colors_are_appended_in_order);
    RUN_TEST(test_init_palette_entered_is_dark_grey_with_yellow_fallback);
    RUN_TEST(test_init_palette_correct_is_green);
    RUN_TEST(test_init_palette_wrong_is_red);
    RUN_TEST(test_init_palette_used_once_is_blue);
    RUN_TEST(test_init_palette_used_twice_is_green);
    RUN_TEST(test_init_palette_used_once_typed_is_bright_blue);
    RUN_TEST(test_init_palette_used_twice_typed_is_bright_green);
    RUN_TEST(test_init_palette_background_is_black);
    RUN_TEST(test_init_palette_replaces_previous_contents);
    RUN_TEST(test_init_palette_is_repeatable);
    RUN_TEST(test_init_palette_after_edit_restores_defaults);
    RUN_TEST(test_palette_copy_is_an_independent_value);
    return test_harness_result();
}
