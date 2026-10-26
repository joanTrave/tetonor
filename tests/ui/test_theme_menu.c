#include <string.h>

#include "../test_harness.h"
#include "../../src/ui/theme_menu.h"

#define TEST_COMMAND_COUNT 5
#define TEST_FAR_THEME 99

static const ThemeMenuCommand all_commands[TEST_COMMAND_COUNT] = {
    THEME_MENU_COMMAND_NONE, THEME_MENU_COMMAND_UP, THEME_MENU_COMMAND_DOWN,
    THEME_MENU_COMMAND_SELECT, THEME_MENU_COMMAND_BACK};

static void walk_to_first_theme(ThemeMenu* menu)
{
    for (int step = 0; step < PALETTE_THEME_COUNT; step++)
    {
        apply_theme_menu_command(menu, THEME_MENU_COMMAND_UP);
    }
}

static void walk_to_last_theme(ThemeMenu* menu)
{
    for (int step = 0; step < PALETTE_THEME_COUNT; step++)
    {
        apply_theme_menu_command(menu, THEME_MENU_COMMAND_DOWN);
    }
}

static void test_init_theme_menu_marks_and_selects_the_active_theme(void)
{
    for (int theme = 0; theme < PALETTE_THEME_COUNT; theme++)
    {
        ThemeMenu menu;
        memset(&menu, 0xA5, sizeof menu);
        init_theme_menu(&menu, theme);
        ASSERT_EQ_INT(menu.active, theme);
        ASSERT_EQ_INT(menu.selected, theme);
        ASSERT_EQ_INT(menu.status, THEME_MENU_OPEN);
    }
}

static void test_init_theme_menu_out_of_range_uses_the_default_theme(void)
{
    const int themes[] = {-1, PALETTE_THEME_COUNT, TEST_FAR_THEME};
    for (size_t i = 0; i < sizeof themes / sizeof themes[0]; i++)
    {
        ThemeMenu menu;
        init_theme_menu(&menu, themes[i]);
        ASSERT_EQ_INT(menu.active, TETONOR_DEFAULT_PALETTE_THEME);
        ASSERT_EQ_INT(menu.selected, TETONOR_DEFAULT_PALETTE_THEME);
        ASSERT_EQ_INT(menu.status, THEME_MENU_OPEN);
    }
}

static void test_init_theme_menu_reopens_a_closed_menu(void)
{
    ThemeMenu menu;
    init_theme_menu(&menu, PALETTE_THEME_CLASSIC);
    apply_theme_menu_command(&menu, THEME_MENU_COMMAND_DOWN);
    apply_theme_menu_command(&menu, THEME_MENU_COMMAND_SELECT);
    ASSERT_EQ_INT(menu.status, THEME_MENU_CHOSEN);
    init_theme_menu(&menu, PALETTE_THEME_LIGHT);
    ASSERT_EQ_INT(menu.active, PALETTE_THEME_LIGHT);
    ASSERT_EQ_INT(menu.selected, PALETTE_THEME_LIGHT);
    ASSERT_EQ_INT(menu.status, THEME_MENU_OPEN);
}

static void test_apply_theme_menu_command_down_moves_to_the_last_theme(void)
{
    ThemeMenu menu;
    init_theme_menu(&menu, PALETTE_THEME_CLASSIC);
    apply_theme_menu_command(&menu, THEME_MENU_COMMAND_DOWN);
    ASSERT_EQ_INT(menu.selected, PALETTE_THEME_CLASSIC + 1);
    ASSERT_EQ_INT(menu.active, PALETTE_THEME_CLASSIC);
    ASSERT_EQ_INT(menu.status, THEME_MENU_OPEN);
    walk_to_last_theme(&menu);
    ASSERT_EQ_INT(menu.selected, PALETTE_THEME_COUNT - 1);
    apply_theme_menu_command(&menu, THEME_MENU_COMMAND_DOWN);
    ASSERT_EQ_INT(menu.selected, PALETTE_THEME_COUNT - 1);
    ASSERT_EQ_INT(menu.status, THEME_MENU_OPEN);
}

static void test_apply_theme_menu_command_up_moves_to_the_first_theme(void)
{
    ThemeMenu menu;
    init_theme_menu(&menu, PALETTE_THEME_COUNT - 1);
    apply_theme_menu_command(&menu, THEME_MENU_COMMAND_UP);
    ASSERT_EQ_INT(menu.selected, PALETTE_THEME_COUNT - 2);
    ASSERT_EQ_INT(menu.active, PALETTE_THEME_COUNT - 1);
    walk_to_first_theme(&menu);
    ASSERT_EQ_INT(menu.selected, 0);
    apply_theme_menu_command(&menu, THEME_MENU_COMMAND_UP);
    ASSERT_EQ_INT(menu.selected, 0);
    ASSERT_EQ_INT(menu.status, THEME_MENU_OPEN);
}

static void test_apply_theme_menu_command_does_not_wrap(void)
{
    ThemeMenu menu;
    init_theme_menu(&menu, PALETTE_THEME_CLASSIC);
    for (int step = 0; step < 3 * PALETTE_THEME_COUNT; step++)
    {
        apply_theme_menu_command(&menu, THEME_MENU_COMMAND_UP);
    }
    ASSERT_EQ_INT(menu.selected, 0);
    for (int step = 0; step < 3 * PALETTE_THEME_COUNT; step++)
    {
        apply_theme_menu_command(&menu, THEME_MENU_COMMAND_DOWN);
    }
    ASSERT_EQ_INT(menu.selected, PALETTE_THEME_COUNT - 1);
}

static void test_apply_theme_menu_command_select_chooses_the_selected(void)
{
    ThemeMenu menu;
    init_theme_menu(&menu, PALETTE_THEME_CLASSIC);
    apply_theme_menu_command(&menu, THEME_MENU_COMMAND_DOWN);
    apply_theme_menu_command(&menu, THEME_MENU_COMMAND_SELECT);
    ASSERT_EQ_INT(menu.status, THEME_MENU_CHOSEN);
    ASSERT_EQ_INT(menu.selected, PALETTE_THEME_CLASSIC + 1);
    ASSERT_EQ_INT(menu.active, PALETTE_THEME_CLASSIC);
}

static void test_apply_theme_menu_command_select_on_active_is_chosen(void)
{
    ThemeMenu menu;
    init_theme_menu(&menu, PALETTE_THEME_LIGHT);
    apply_theme_menu_command(&menu, THEME_MENU_COMMAND_SELECT);
    ASSERT_EQ_INT(menu.status, THEME_MENU_CHOSEN);
    ASSERT_EQ_INT(menu.selected, PALETTE_THEME_LIGHT);
    ASSERT_EQ_INT(menu.active, PALETTE_THEME_LIGHT);
}

static void test_apply_theme_menu_command_back_cancels_keeping_selection(void)
{
    ThemeMenu menu;
    init_theme_menu(&menu, PALETTE_THEME_CLASSIC);
    apply_theme_menu_command(&menu, THEME_MENU_COMMAND_DOWN);
    apply_theme_menu_command(&menu, THEME_MENU_COMMAND_BACK);
    ASSERT_EQ_INT(menu.status, THEME_MENU_CANCELLED);
    ASSERT_EQ_INT(menu.selected, PALETTE_THEME_CLASSIC + 1);
    ASSERT_EQ_INT(menu.active, PALETTE_THEME_CLASSIC);
}

static void test_apply_theme_menu_command_none_changes_nothing(void)
{
    ThemeMenu menu;
    init_theme_menu(&menu, PALETTE_THEME_LIGHT);
    apply_theme_menu_command(&menu, THEME_MENU_COMMAND_NONE);
    ASSERT_EQ_INT(menu.active, PALETTE_THEME_LIGHT);
    ASSERT_EQ_INT(menu.selected, PALETTE_THEME_LIGHT);
    ASSERT_EQ_INT(menu.status, THEME_MENU_OPEN);
}

static void test_apply_theme_menu_command_ignored_once_chosen(void)
{
    for (int command = 0; command < TEST_COMMAND_COUNT; command++)
    {
        ThemeMenu menu;
        init_theme_menu(&menu, PALETTE_THEME_CLASSIC);
        apply_theme_menu_command(&menu, THEME_MENU_COMMAND_DOWN);
        apply_theme_menu_command(&menu, THEME_MENU_COMMAND_SELECT);
        apply_theme_menu_command(&menu, all_commands[command]);
        ASSERT_EQ_INT(menu.status, THEME_MENU_CHOSEN);
        ASSERT_EQ_INT(menu.selected, PALETTE_THEME_CLASSIC + 1);
        ASSERT_EQ_INT(menu.active, PALETTE_THEME_CLASSIC);
    }
}

static void test_apply_theme_menu_command_ignored_once_cancelled(void)
{
    for (int command = 0; command < TEST_COMMAND_COUNT; command++)
    {
        ThemeMenu menu;
        init_theme_menu(&menu, PALETTE_THEME_CLASSIC);
        apply_theme_menu_command(&menu, THEME_MENU_COMMAND_BACK);
        apply_theme_menu_command(&menu, all_commands[command]);
        ASSERT_EQ_INT(menu.status, THEME_MENU_CANCELLED);
        ASSERT_EQ_INT(menu.selected, PALETTE_THEME_CLASSIC);
        ASSERT_EQ_INT(menu.active, PALETTE_THEME_CLASSIC);
    }
}

static void test_theme_menu_label_names_every_theme(void)
{
    ASSERT_TRUE(strcmp(theme_menu_label(PALETTE_THEME_CLASSIC), "Classic") ==
                0);
    ASSERT_TRUE(strcmp(theme_menu_label(PALETTE_THEME_LIGHT), "Light") == 0);
}

static void test_theme_menu_label_clamps_out_of_range_values(void)
{
    ASSERT_TRUE(strcmp(theme_menu_label(-1),
                       theme_menu_label(PALETTE_THEME_CLASSIC)) == 0);
    ASSERT_TRUE(strcmp(theme_menu_label(PALETTE_THEME_COUNT),
                       theme_menu_label(PALETTE_THEME_COUNT - 1)) == 0);
    ASSERT_TRUE(strcmp(theme_menu_label(TEST_FAR_THEME),
                       theme_menu_label(PALETTE_THEME_COUNT - 1)) == 0);
}

static void test_theme_menu_labels_are_distinct_and_not_empty(void)
{
    for (int a = 0; a < PALETTE_THEME_COUNT; a++)
    {
        ASSERT_TRUE(theme_menu_label(a)[0] != '\0');
        for (int b = a + 1; b < PALETTE_THEME_COUNT; b++)
        {
            ASSERT_TRUE(strcmp(theme_menu_label(a), theme_menu_label(b)) != 0);
        }
    }
}

static void test_theme_menu_marker_marks_only_the_active_theme(void)
{
    for (int active = 0; active < PALETTE_THEME_COUNT; active++)
    {
        ThemeMenu menu;
        init_theme_menu(&menu, active);
        for (int theme = 0; theme < PALETTE_THEME_COUNT; theme++)
        {
            const char* expected =
                theme == active ? THEME_MENU_ACTIVE_MARKER : "";
            ASSERT_TRUE(strcmp(theme_menu_marker(&menu, theme), expected) == 0);
        }
    }
}

static void test_theme_menu_marker_follows_active_not_selected(void)
{
    ThemeMenu menu;
    init_theme_menu(&menu, PALETTE_THEME_CLASSIC);
    apply_theme_menu_command(&menu, THEME_MENU_COMMAND_DOWN);
    ASSERT_TRUE(strcmp(theme_menu_marker(&menu, PALETTE_THEME_CLASSIC),
                       THEME_MENU_ACTIVE_MARKER) == 0);
    ASSERT_TRUE(strcmp(theme_menu_marker(&menu, PALETTE_THEME_LIGHT), "") == 0);
}

static void test_theme_menu_active_marker_text(void)
{
    ASSERT_TRUE(strcmp(THEME_MENU_ACTIVE_MARKER, " (current)") == 0);
}

int main(void)
{
    RUN_TEST(test_init_theme_menu_marks_and_selects_the_active_theme);
    RUN_TEST(test_init_theme_menu_out_of_range_uses_the_default_theme);
    RUN_TEST(test_init_theme_menu_reopens_a_closed_menu);
    RUN_TEST(test_apply_theme_menu_command_down_moves_to_the_last_theme);
    RUN_TEST(test_apply_theme_menu_command_up_moves_to_the_first_theme);
    RUN_TEST(test_apply_theme_menu_command_does_not_wrap);
    RUN_TEST(test_apply_theme_menu_command_select_chooses_the_selected);
    RUN_TEST(test_apply_theme_menu_command_select_on_active_is_chosen);
    RUN_TEST(test_apply_theme_menu_command_back_cancels_keeping_selection);
    RUN_TEST(test_apply_theme_menu_command_none_changes_nothing);
    RUN_TEST(test_apply_theme_menu_command_ignored_once_chosen);
    RUN_TEST(test_apply_theme_menu_command_ignored_once_cancelled);
    RUN_TEST(test_theme_menu_label_names_every_theme);
    RUN_TEST(test_theme_menu_label_clamps_out_of_range_values);
    RUN_TEST(test_theme_menu_labels_are_distinct_and_not_empty);
    RUN_TEST(test_theme_menu_marker_marks_only_the_active_theme);
    RUN_TEST(test_theme_menu_marker_follows_active_not_selected);
    RUN_TEST(test_theme_menu_active_marker_text);
    return test_harness_result();
}
