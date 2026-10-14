#include <stdbool.h>

#include "../test_harness.h"
#include "../../src/ui/palette_style.h"

#define TEST_BASIC_COLOR_COUNT 8
#define TEST_COUNT_LEVELS 4
#define TEST_BELOW_BASIC_COUNT 7
#define TEST_BELOW_16_COUNT 15
#define TEST_MANY_COLORS 256
#define TEST_VERY_MANY_COLORS 768
#define TEST_STRIP_ROLE_COUNT 5
#define TEST_USED_STATE_COUNT 4

static const int fixture_color_counts[TEST_COUNT_LEVELS] = {
    TETONOR_BASIC_COLOR_COUNT, TETONOR_16_COLOR_COUNT, TEST_MANY_COLORS,
    TEST_VERY_MANY_COLORS};
static const PaletteColor fixture_basic_colors[TEST_BASIC_COLOR_COUNT] = {
    PALETTE_COLOR_BLACK,  PALETTE_COLOR_RED,  PALETTE_COLOR_GREEN,
    PALETTE_COLOR_YELLOW, PALETTE_COLOR_BLUE, PALETTE_COLOR_MAGENTA,
    PALETTE_COLOR_CYAN,   PALETTE_COLOR_WHITE};
static const PaletteRole fixture_strip_roles[TEST_STRIP_ROLE_COUNT] = {
    PALETTE_ROLE_ENTERED, PALETTE_ROLE_USED_ONCE, PALETTE_ROLE_USED_TWICE,
    PALETTE_ROLE_USED_ONCE_TYPED, PALETTE_ROLE_USED_TWICE_TYPED};

static PaletteEntry make_entry(PaletteColor color, PaletteColor fallback)
{
    PaletteEntry entry = {.color = color, .fallback = fallback};
    return entry;
}

static PaletteColor resolve_default(PaletteRole role, int color_count)
{
    Palette palette;
    init_palette(&palette);
    return resolve_palette_color(palette.foregrounds[role], color_count);
}

static bool is_given_typed_pair(PaletteRole first, PaletteRole second)
{
    return (first == PALETTE_ROLE_USED_ONCE &&
            second == PALETTE_ROLE_USED_ONCE_TYPED) ||
           (first == PALETTE_ROLE_USED_TWICE &&
            second == PALETTE_ROLE_USED_TWICE_TYPED);
}

static void test_color_count_thresholds_match_the_tiers(void)
{
    ASSERT_EQ_INT(TETONOR_BASIC_COLOR_COUNT, 8);
    ASSERT_EQ_INT(TETONOR_16_COLOR_COUNT, 16);
}

static void test_resolve_palette_color_basic_color_is_kept_at_every_count(void)
{
    for (int c = 0; c < TEST_BASIC_COLOR_COUNT; c++)
    {
        for (int n = 0; n < TEST_COUNT_LEVELS; n++)
        {
            PaletteEntry entry =
                make_entry(fixture_basic_colors[c], PALETTE_COLOR_DARK_GREY);
            ASSERT_EQ_INT(resolve_palette_color(entry, fixture_color_counts[n]),
                          fixture_basic_colors[c]);
        }
    }
}

static void test_resolve_palette_color_below_basic_count_uses_fallback(void)
{
    PaletteEntry entry = make_entry(PALETTE_COLOR_RED, PALETTE_COLOR_BLUE);
    ASSERT_EQ_INT(resolve_palette_color(entry, TEST_BELOW_BASIC_COUNT),
                  PALETTE_COLOR_BLUE);
}

static void test_resolve_palette_color_dark_grey_needs_sixteen_colors(void)
{
    PaletteEntry entry =
        make_entry(PALETTE_COLOR_DARK_GREY, PALETTE_COLOR_YELLOW);
    ASSERT_EQ_INT(resolve_palette_color(entry, TETONOR_BASIC_COLOR_COUNT),
                  PALETTE_COLOR_YELLOW);
    ASSERT_EQ_INT(resolve_palette_color(entry, TEST_BELOW_16_COUNT),
                  PALETTE_COLOR_YELLOW);
    ASSERT_EQ_INT(resolve_palette_color(entry, TETONOR_16_COLOR_COUNT),
                  PALETTE_COLOR_DARK_GREY);
}

static void test_resolve_palette_color_dark_grey_is_kept_above_sixteen(void)
{
    PaletteEntry entry =
        make_entry(PALETTE_COLOR_DARK_GREY, PALETTE_COLOR_YELLOW);
    ASSERT_EQ_INT(resolve_palette_color(entry, TEST_MANY_COLORS),
                  PALETTE_COLOR_DARK_GREY);
    ASSERT_EQ_INT(resolve_palette_color(entry, TEST_VERY_MANY_COLORS),
                  PALETTE_COLOR_DARK_GREY);
}

static void test_resolve_palette_color_bright_blue_needs_sixteen_colors(void)
{
    PaletteEntry entry =
        make_entry(PALETTE_COLOR_BRIGHT_BLUE, PALETTE_COLOR_BLUE);
    ASSERT_EQ_INT(resolve_palette_color(entry, TETONOR_BASIC_COLOR_COUNT),
                  PALETTE_COLOR_BLUE);
    ASSERT_EQ_INT(resolve_palette_color(entry, TEST_BELOW_16_COUNT),
                  PALETTE_COLOR_BLUE);
    ASSERT_EQ_INT(resolve_palette_color(entry, TETONOR_16_COLOR_COUNT),
                  PALETTE_COLOR_BRIGHT_BLUE);
    ASSERT_EQ_INT(resolve_palette_color(entry, TEST_MANY_COLORS),
                  PALETTE_COLOR_BRIGHT_BLUE);
    ASSERT_EQ_INT(resolve_palette_color(entry, TEST_VERY_MANY_COLORS),
                  PALETTE_COLOR_BRIGHT_BLUE);
}

static void test_resolve_palette_color_bright_green_needs_sixteen_colors(void)
{
    PaletteEntry entry =
        make_entry(PALETTE_COLOR_BRIGHT_GREEN, PALETTE_COLOR_GREEN);
    ASSERT_EQ_INT(resolve_palette_color(entry, TETONOR_BASIC_COLOR_COUNT),
                  PALETTE_COLOR_GREEN);
    ASSERT_EQ_INT(resolve_palette_color(entry, TEST_BELOW_16_COUNT),
                  PALETTE_COLOR_GREEN);
    ASSERT_EQ_INT(resolve_palette_color(entry, TETONOR_16_COLOR_COUNT),
                  PALETTE_COLOR_BRIGHT_GREEN);
    ASSERT_EQ_INT(resolve_palette_color(entry, TEST_MANY_COLORS),
                  PALETTE_COLOR_BRIGHT_GREEN);
    ASSERT_EQ_INT(resolve_palette_color(entry, TEST_VERY_MANY_COLORS),
                  PALETTE_COLOR_BRIGHT_GREEN);
}

static void test_default_fallbacks_are_basic_colors(void)
{
    Palette palette;
    init_palette(&palette);
    for (int role = 0; role < PALETTE_ROLE_COUNT; role++)
    {
        ASSERT_TRUE(palette.foregrounds[role].fallback <= PALETTE_COLOR_WHITE);
    }
    ASSERT_TRUE(palette.background.fallback <= PALETTE_COLOR_WHITE);
}

static void test_default_strip_roles_have_distinct_colors_from_16_colors(void)
{
    for (int n = 1; n < TEST_COUNT_LEVELS; n++)
    {
        for (int a = 0; a < TEST_STRIP_ROLE_COUNT; a++)
        {
            for (int b = a + 1; b < TEST_STRIP_ROLE_COUNT; b++)
            {
                ASSERT_TRUE(resolve_default(fixture_strip_roles[a],
                                            fixture_color_counts[n]) !=
                            resolve_default(fixture_strip_roles[b],
                                            fixture_color_counts[n]));
            }
        }
    }
}

static void
test_default_strip_roles_collide_at_basic_colors_only_given_typed(void)
{
    for (int a = 0; a < TEST_STRIP_ROLE_COUNT; a++)
    {
        for (int b = a + 1; b < TEST_STRIP_ROLE_COUNT; b++)
        {
            bool same = resolve_default(fixture_strip_roles[a],
                                        TETONOR_BASIC_COLOR_COUNT) ==
                        resolve_default(fixture_strip_roles[b],
                                        TETONOR_BASIC_COLOR_COUNT);
            ASSERT_EQ_INT(same, is_given_typed_pair(fixture_strip_roles[a],
                                                    fixture_strip_roles[b]));
        }
    }
}

static void test_default_typed_roles_are_bright_from_16_colors(void)
{
    for (int n = 1; n < TEST_COUNT_LEVELS; n++)
    {
        ASSERT_EQ_INT(resolve_default(PALETTE_ROLE_USED_ONCE_TYPED,
                                      fixture_color_counts[n]),
                      PALETTE_COLOR_BRIGHT_BLUE);
        ASSERT_EQ_INT(resolve_default(PALETTE_ROLE_USED_TWICE_TYPED,
                                      fixture_color_counts[n]),
                      PALETTE_COLOR_BRIGHT_GREEN);
    }
}

static void test_default_typed_roles_equal_given_roles_at_basic_colors(void)
{
    ASSERT_EQ_INT(resolve_default(PALETTE_ROLE_USED_ONCE_TYPED,
                                  TETONOR_BASIC_COLOR_COUNT),
                  PALETTE_COLOR_BLUE);
    ASSERT_EQ_INT(resolve_default(PALETTE_ROLE_USED_TWICE_TYPED,
                                  TETONOR_BASIC_COLOR_COUNT),
                  PALETTE_COLOR_GREEN);
}

static void test_default_entered_is_not_blue_at_basic_colors(void)
{
    PaletteColor entered =
        resolve_default(PALETTE_ROLE_ENTERED, TETONOR_BASIC_COLOR_COUNT);
    ASSERT_TRUE(entered != PALETTE_COLOR_BLUE);
    ASSERT_TRUE(entered != PALETTE_COLOR_GREEN);
}

static void test_default_entered_is_dark_grey_from_sixteen_colors(void)
{
    ASSERT_EQ_INT(resolve_default(PALETTE_ROLE_ENTERED, TETONOR_16_COLOR_COUNT),
                  PALETTE_COLOR_DARK_GREY);
    ASSERT_EQ_INT(resolve_default(PALETTE_ROLE_ENTERED, TEST_MANY_COLORS),
                  PALETTE_COLOR_DARK_GREY);
}

static void test_default_used_once_is_blue_at_every_count(void)
{
    for (int n = 0; n < TEST_COUNT_LEVELS; n++)
    {
        ASSERT_EQ_INT(
            resolve_default(PALETTE_ROLE_USED_ONCE, fixture_color_counts[n]),
            PALETTE_COLOR_BLUE);
    }
}

static void test_default_used_twice_is_green_at_every_count(void)
{
    for (int n = 0; n < TEST_COUNT_LEVELS; n++)
    {
        ASSERT_EQ_INT(
            resolve_default(PALETTE_ROLE_USED_TWICE, fixture_color_counts[n]),
            PALETTE_COLOR_GREEN);
    }
}

static void test_default_result_roles_keep_their_colors(void)
{
    for (int n = 0; n < TEST_COUNT_LEVELS; n++)
    {
        ASSERT_EQ_INT(
            resolve_default(PALETTE_ROLE_CORRECT, fixture_color_counts[n]),
            PALETTE_COLOR_GREEN);
        ASSERT_EQ_INT(
            resolve_default(PALETTE_ROLE_WRONG, fixture_color_counts[n]),
            PALETTE_COLOR_RED);
    }
}

static void test_default_background_is_black_at_every_count(void)
{
    Palette palette;
    init_palette(&palette);
    for (int n = 0; n < TEST_COUNT_LEVELS; n++)
    {
        ASSERT_EQ_INT(
            resolve_palette_color(palette.background, fixture_color_counts[n]),
            PALETTE_COLOR_BLACK);
    }
}

static void test_strip_slot_role_given_unused_keeps_default_attribute(void)
{
    PaletteRole role = PALETTE_ROLE_COUNT;
    ASSERT_FALSE(strip_slot_role(false, SLOT_USAGE_NONE, &role));
}

static void test_strip_slot_role_typed_unused_is_entered(void)
{
    PaletteRole role = PALETTE_ROLE_COUNT;
    ASSERT_TRUE(strip_slot_role(true, SLOT_USAGE_NONE, &role));
    ASSERT_EQ_INT(role, PALETTE_ROLE_ENTERED);
}

static void test_strip_slot_role_given_once_is_used_once(void)
{
    PaletteRole role = PALETTE_ROLE_COUNT;
    ASSERT_TRUE(strip_slot_role(false, SLOT_USAGE_ONCE, &role));
    ASSERT_EQ_INT(role, PALETTE_ROLE_USED_ONCE);
}

static void test_strip_slot_role_given_twice_is_used_twice(void)
{
    PaletteRole role = PALETTE_ROLE_COUNT;
    ASSERT_TRUE(strip_slot_role(false, SLOT_USAGE_TWICE, &role));
    ASSERT_EQ_INT(role, PALETTE_ROLE_USED_TWICE);
}

static void test_strip_slot_role_typed_once_is_used_once_typed(void)
{
    PaletteRole role = PALETTE_ROLE_COUNT;
    ASSERT_TRUE(strip_slot_role(true, SLOT_USAGE_ONCE, &role));
    ASSERT_EQ_INT(role, PALETTE_ROLE_USED_ONCE_TYPED);
}

static void test_strip_slot_role_typed_twice_is_used_twice_typed(void)
{
    PaletteRole role = PALETTE_ROLE_COUNT;
    ASSERT_TRUE(strip_slot_role(true, SLOT_USAGE_TWICE, &role));
    ASSERT_EQ_INT(role, PALETTE_ROLE_USED_TWICE_TYPED);
}

static void test_strip_slot_role_used_states_never_share_a_role(void)
{
    PaletteRole roles[TEST_USED_STATE_COUNT];
    const bool editable[TEST_USED_STATE_COUNT]    = {false, false, true, true};
    const SlotUsage usages[TEST_USED_STATE_COUNT] = {
        SLOT_USAGE_ONCE, SLOT_USAGE_TWICE, SLOT_USAGE_ONCE, SLOT_USAGE_TWICE};
    for (int i = 0; i < TEST_USED_STATE_COUNT; i++)
    {
        ASSERT_TRUE(strip_slot_role(editable[i], usages[i], &roles[i]));
    }
    for (int a = 0; a < TEST_USED_STATE_COUNT; a++)
    {
        for (int b = a + 1; b < TEST_USED_STATE_COUNT; b++)
        {
            ASSERT_TRUE(roles[a] != roles[b]);
        }
    }
}

int main(void)
{
    RUN_TEST(test_color_count_thresholds_match_the_tiers);
    RUN_TEST(test_resolve_palette_color_basic_color_is_kept_at_every_count);
    RUN_TEST(test_resolve_palette_color_below_basic_count_uses_fallback);
    RUN_TEST(test_resolve_palette_color_dark_grey_needs_sixteen_colors);
    RUN_TEST(test_resolve_palette_color_dark_grey_is_kept_above_sixteen);
    RUN_TEST(test_resolve_palette_color_bright_blue_needs_sixteen_colors);
    RUN_TEST(test_resolve_palette_color_bright_green_needs_sixteen_colors);
    RUN_TEST(test_default_fallbacks_are_basic_colors);
    RUN_TEST(test_default_strip_roles_have_distinct_colors_from_16_colors);
    RUN_TEST(test_default_strip_roles_collide_at_basic_colors_only_given_typed);
    RUN_TEST(test_default_typed_roles_are_bright_from_16_colors);
    RUN_TEST(test_default_typed_roles_equal_given_roles_at_basic_colors);
    RUN_TEST(test_default_entered_is_not_blue_at_basic_colors);
    RUN_TEST(test_default_entered_is_dark_grey_from_sixteen_colors);
    RUN_TEST(test_default_used_once_is_blue_at_every_count);
    RUN_TEST(test_default_used_twice_is_green_at_every_count);
    RUN_TEST(test_default_result_roles_keep_their_colors);
    RUN_TEST(test_default_background_is_black_at_every_count);
    RUN_TEST(test_strip_slot_role_given_unused_keeps_default_attribute);
    RUN_TEST(test_strip_slot_role_typed_unused_is_entered);
    RUN_TEST(test_strip_slot_role_given_once_is_used_once);
    RUN_TEST(test_strip_slot_role_given_twice_is_used_twice);
    RUN_TEST(test_strip_slot_role_typed_once_is_used_once_typed);
    RUN_TEST(test_strip_slot_role_typed_twice_is_used_twice_typed);
    RUN_TEST(test_strip_slot_role_used_states_never_share_a_role);
    return test_harness_result();
}
