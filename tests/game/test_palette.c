#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/game/palette.h"

#define TEST_ROLE_COUNT 9
#define TEST_BASIC_COLOR_COUNT 8
#define TEST_GARBAGE_BYTE 0xA5
#define TEST_FAR_THEME 99

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

static void test_init_palette_conflict_is_magenta(void)
{
    Palette palette;
    init_palette(&palette);
    ASSERT_TRUE(entry_is(&palette.foregrounds[PALETTE_ROLE_CONFLICT],
                         PALETTE_COLOR_MAGENTA, PALETTE_COLOR_MAGENTA));
}

static void test_palette_conflict_role_is_appended_last(void)
{
    ASSERT_EQ_INT(PALETTE_ROLE_CONFLICT, PALETTE_ROLE_COUNT - 1);
}

static void test_init_palette_conflict_differs_from_every_other_role(void)
{
    Palette palette;
    init_palette(&palette);
    for (int role = 0; role < PALETTE_ROLE_COUNT; role++)
    {
        if (role == PALETTE_ROLE_CONFLICT)
        {
            continue;
        }
        ASSERT_FALSE(
            entries_equal(&palette.foregrounds[role],
                          &palette.foregrounds[PALETTE_ROLE_CONFLICT]));
        ASSERT_TRUE(palette.foregrounds[role].color !=
                    palette.foregrounds[PALETTE_ROLE_CONFLICT].color);
    }
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

static void test_palette_bright_white_is_appended(void)
{
    ASSERT_EQ_INT(PALETTE_COLOR_BRIGHT_WHITE, PALETTE_COLOR_BRIGHT_GREEN + 1);
}

static void test_palette_text_role_is_appended(void)
{
    ASSERT_EQ_INT(PALETTE_ROLE_TEXT, PALETTE_ROLE_USED_TWICE_TYPED + 1);
    ASSERT_EQ_INT(PALETTE_ROLE_CONFLICT, PALETTE_ROLE_TEXT + 1);
}

static void test_is_palette_theme_accepts_only_themes(void)
{
    ASSERT_FALSE(is_palette_theme(-1));
    ASSERT_TRUE(is_palette_theme(PALETTE_THEME_CLASSIC));
    ASSERT_TRUE(is_palette_theme(PALETTE_THEME_LIGHT));
    ASSERT_FALSE(is_palette_theme(PALETTE_THEME_COUNT));
    ASSERT_FALSE(is_palette_theme(TEST_FAR_THEME));
}

static void test_palette_has_at_least_two_themes(void)
{
    ASSERT_TRUE(PALETTE_THEME_COUNT >= 2);
    ASSERT_EQ_INT(PALETTE_THEME_CLASSIC, 0);
    ASSERT_EQ_INT(TETONOR_DEFAULT_PALETTE_THEME, PALETTE_THEME_CLASSIC);
}

static void test_init_theme_palette_classic_equals_init_palette(void)
{
    Palette expected;
    Palette palette;
    init_palette(&expected);
    init_theme_palette(&palette, PALETTE_THEME_CLASSIC);
    ASSERT_TRUE(palettes_equal(&palette, &expected));
}

static void test_init_theme_palette_classic_is_todays_look(void)
{
    Palette palette;
    init_theme_palette(&palette, PALETTE_THEME_CLASSIC);
    const PaletteEntry* roles = palette.foregrounds;
    ASSERT_TRUE(entry_is(&roles[PALETTE_ROLE_ENTERED], PALETTE_COLOR_DARK_GREY,
                         PALETTE_COLOR_YELLOW));
    ASSERT_TRUE(entry_is(&roles[PALETTE_ROLE_CORRECT], PALETTE_COLOR_GREEN,
                         PALETTE_COLOR_GREEN));
    ASSERT_TRUE(entry_is(&roles[PALETTE_ROLE_WRONG], PALETTE_COLOR_RED,
                         PALETTE_COLOR_RED));
    ASSERT_TRUE(entry_is(&roles[PALETTE_ROLE_USED_ONCE], PALETTE_COLOR_BLUE,
                         PALETTE_COLOR_BLUE));
    ASSERT_TRUE(entry_is(&roles[PALETTE_ROLE_USED_TWICE], PALETTE_COLOR_GREEN,
                         PALETTE_COLOR_GREEN));
    ASSERT_TRUE(entry_is(&roles[PALETTE_ROLE_USED_ONCE_TYPED],
                         PALETTE_COLOR_BRIGHT_BLUE, PALETTE_COLOR_BLUE));
    ASSERT_TRUE(entry_is(&roles[PALETTE_ROLE_USED_TWICE_TYPED],
                         PALETTE_COLOR_BRIGHT_GREEN, PALETTE_COLOR_GREEN));
    ASSERT_TRUE(entry_is(&roles[PALETTE_ROLE_TEXT], PALETTE_COLOR_WHITE,
                         PALETTE_COLOR_WHITE));
    ASSERT_TRUE(entry_is(&palette.background, PALETTE_COLOR_BLACK,
                         PALETTE_COLOR_BLACK));
}

static void test_init_theme_palette_light_values(void)
{
    Palette palette;
    init_theme_palette(&palette, PALETTE_THEME_LIGHT);
    const PaletteEntry* roles = palette.foregrounds;
    ASSERT_TRUE(entry_is(&roles[PALETTE_ROLE_ENTERED], PALETTE_COLOR_MAGENTA,
                         PALETTE_COLOR_MAGENTA));
    ASSERT_TRUE(entry_is(&roles[PALETTE_ROLE_CORRECT], PALETTE_COLOR_GREEN,
                         PALETTE_COLOR_GREEN));
    ASSERT_TRUE(entry_is(&roles[PALETTE_ROLE_WRONG], PALETTE_COLOR_RED,
                         PALETTE_COLOR_RED));
    ASSERT_TRUE(entry_is(&roles[PALETTE_ROLE_USED_ONCE], PALETTE_COLOR_BLUE,
                         PALETTE_COLOR_BLUE));
    ASSERT_TRUE(entry_is(&roles[PALETTE_ROLE_USED_TWICE], PALETTE_COLOR_GREEN,
                         PALETTE_COLOR_GREEN));
    ASSERT_TRUE(entry_is(&roles[PALETTE_ROLE_USED_ONCE_TYPED],
                         PALETTE_COLOR_BRIGHT_BLUE, PALETTE_COLOR_BLUE));
    ASSERT_TRUE(entry_is(&roles[PALETTE_ROLE_USED_TWICE_TYPED],
                         PALETTE_COLOR_CYAN, PALETTE_COLOR_CYAN));
    ASSERT_TRUE(entry_is(&roles[PALETTE_ROLE_TEXT], PALETTE_COLOR_BLACK,
                         PALETTE_COLOR_BLACK));
    ASSERT_TRUE(entry_is(&palette.background, PALETTE_COLOR_BRIGHT_WHITE,
                         PALETTE_COLOR_WHITE));
}

static void test_init_theme_palette_conflict_role_per_theme(void)
{
    Palette classic;
    Palette light;
    init_theme_palette(&classic, PALETTE_THEME_CLASSIC);
    init_theme_palette(&light, PALETTE_THEME_LIGHT);
    ASSERT_TRUE(entry_is(&classic.foregrounds[PALETTE_ROLE_CONFLICT],
                         PALETTE_COLOR_MAGENTA, PALETTE_COLOR_MAGENTA));
    ASSERT_TRUE(entry_is(&light.foregrounds[PALETTE_ROLE_CONFLICT],
                         PALETTE_COLOR_DARK_GREY, PALETTE_COLOR_RED));
}

/* The one intended overlap: on the Light theme the 8-colour conflict fallback
 * is red, shared with WRONG (both mean error; the "!" marker tells them
 * apart). Every other role and every other theme must stay fully distinct. */
static bool conflict_fallback_overlap_is_intended(int theme, int role)
{
    return theme == PALETTE_THEME_LIGHT && role == PALETTE_ROLE_WRONG;
}

static void test_theme_conflict_differs_from_every_other_role(void)
{
    for (int theme = 0; theme < PALETTE_THEME_COUNT; theme++)
    {
        Palette palette;
        init_theme_palette(&palette, theme);
        for (int role = 0; role < PALETTE_ROLE_COUNT; role++)
        {
            if (role == PALETTE_ROLE_CONFLICT)
            {
                continue;
            }
            ASSERT_TRUE(palette.foregrounds[role].color !=
                        palette.foregrounds[PALETTE_ROLE_CONFLICT].color);
            if (!conflict_fallback_overlap_is_intended(theme, role))
            {
                ASSERT_TRUE(
                    palette.foregrounds[role].fallback !=
                    palette.foregrounds[PALETTE_ROLE_CONFLICT].fallback);
            }
        }
    }
}

static void test_light_conflict_fallback_is_shared_with_wrong_only(void)
{
    Palette palette;
    init_theme_palette(&palette, PALETTE_THEME_LIGHT);
    PaletteColor conflict = palette.foregrounds[PALETTE_ROLE_CONFLICT].fallback;
    ASSERT_EQ_INT(conflict, palette.foregrounds[PALETTE_ROLE_WRONG].fallback);
    int sharing = 0;
    for (int role = 0; role < PALETTE_ROLE_COUNT; role++)
    {
        if (palette.foregrounds[role].fallback == conflict)
        {
            sharing++;
        }
    }
    /* CONFLICT itself and WRONG, nothing else. */
    ASSERT_EQ_INT(sharing, 2);
}

static void test_light_conflict_fallback_is_readable_on_background(void)
{
    Palette palette;
    init_theme_palette(&palette, PALETTE_THEME_LIGHT);
    PaletteColor conflict = palette.foregrounds[PALETTE_ROLE_CONFLICT].fallback;
    ASSERT_TRUE(conflict != palette.background.fallback);
    ASSERT_TRUE(conflict != PALETTE_COLOR_WHITE);
    ASSERT_TRUE(conflict != PALETTE_COLOR_YELLOW);
}

static void test_init_theme_palette_out_of_range_is_default(void)
{
    Palette expected;
    init_palette(&expected);
    const int themes[] = {-1, PALETTE_THEME_COUNT, TEST_FAR_THEME};
    for (size_t i = 0; i < sizeof themes / sizeof themes[0]; i++)
    {
        Palette palette;
        memset(&palette, TEST_GARBAGE_BYTE, sizeof palette);
        init_theme_palette(&palette, themes[i]);
        ASSERT_TRUE(palettes_equal(&palette, &expected));
    }
}

static void test_init_theme_palette_replaces_previous_contents(void)
{
    for (int theme = 0; theme < PALETTE_THEME_COUNT; theme++)
    {
        Palette first;
        Palette second;
        init_theme_palette(&first, theme);
        memset(&second, TEST_GARBAGE_BYTE, sizeof second);
        init_theme_palette(&second, theme);
        ASSERT_TRUE(palettes_equal(&first, &second));
    }
}

static void test_themes_have_different_palettes(void)
{
    for (int a = 0; a < PALETTE_THEME_COUNT; a++)
    {
        for (int b = a + 1; b < PALETTE_THEME_COUNT; b++)
        {
            Palette first;
            Palette second;
            init_theme_palette(&first, a);
            init_theme_palette(&second, b);
            ASSERT_FALSE(palettes_equal(&first, &second));
            ASSERT_FALSE(entries_equal(&first.background, &second.background));
        }
    }
}

static void test_theme_fallbacks_are_basic_colors(void)
{
    for (int theme = 0; theme < PALETTE_THEME_COUNT; theme++)
    {
        Palette palette;
        init_theme_palette(&palette, theme);
        for (int role = 0; role < PALETTE_ROLE_COUNT; role++)
        {
            ASSERT_TRUE(palette.foregrounds[role].fallback <=
                        PALETTE_COLOR_WHITE);
        }
        ASSERT_TRUE(palette.background.fallback <= PALETTE_COLOR_WHITE);
    }
}

static void test_theme_preferred_roles_differ_from_background(void)
{
    for (int theme = 0; theme < PALETTE_THEME_COUNT; theme++)
    {
        Palette palette;
        init_theme_palette(&palette, theme);
        for (int role = 0; role < PALETTE_ROLE_COUNT; role++)
        {
            ASSERT_TRUE(palette.foregrounds[role].color !=
                        palette.background.color);
            ASSERT_TRUE(palette.foregrounds[role].fallback !=
                        palette.background.fallback);
        }
    }
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
    RUN_TEST(test_init_palette_conflict_is_magenta);
    RUN_TEST(test_palette_conflict_role_is_appended_last);
    RUN_TEST(test_init_palette_conflict_differs_from_every_other_role);
    RUN_TEST(test_init_palette_background_is_black);
    RUN_TEST(test_init_palette_replaces_previous_contents);
    RUN_TEST(test_init_palette_is_repeatable);
    RUN_TEST(test_init_palette_after_edit_restores_defaults);
    RUN_TEST(test_palette_copy_is_an_independent_value);
    RUN_TEST(test_palette_bright_white_is_appended);
    RUN_TEST(test_palette_text_role_is_appended);
    RUN_TEST(test_is_palette_theme_accepts_only_themes);
    RUN_TEST(test_palette_has_at_least_two_themes);
    RUN_TEST(test_init_theme_palette_classic_equals_init_palette);
    RUN_TEST(test_init_theme_palette_classic_is_todays_look);
    RUN_TEST(test_init_theme_palette_light_values);
    RUN_TEST(test_init_theme_palette_conflict_role_per_theme);
    RUN_TEST(test_theme_conflict_differs_from_every_other_role);
    RUN_TEST(test_light_conflict_fallback_is_shared_with_wrong_only);
    RUN_TEST(test_light_conflict_fallback_is_readable_on_background);
    RUN_TEST(test_init_theme_palette_out_of_range_is_default);
    RUN_TEST(test_init_theme_palette_replaces_previous_contents);
    RUN_TEST(test_themes_have_different_palettes);
    RUN_TEST(test_theme_fallbacks_are_basic_colors);
    RUN_TEST(test_theme_preferred_roles_differ_from_background);
    return test_harness_result();
}
