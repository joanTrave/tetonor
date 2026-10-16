#include <stddef.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/ui/difficulty_menu.h"

#define TEST_COMMAND_COUNT 5
#define TEST_BELOW_RANGE (-1)
#define TEST_ABOVE_RANGE 3

static const DifficultyMenuCommand all_commands[TEST_COMMAND_COUNT] = {
    DIFFICULTY_COMMAND_NONE, DIFFICULTY_COMMAND_UP, DIFFICULTY_COMMAND_DOWN,
    DIFFICULTY_COMMAND_CONFIRM, DIFFICULTY_COMMAND_BACK};

static void test_init_difficulty_menu_selects_medium_and_is_open(void)
{
    DifficultyMenu menu;
    init_difficulty_menu(&menu);
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_MEDIUM);
    ASSERT_EQ_INT(menu.status, DIFFICULTY_MENU_OPEN);
}

static void test_apply_difficulty_menu_command_up_moves_towards_easy(void)
{
    DifficultyMenu menu;
    init_difficulty_menu(&menu);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_UP);
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_EASY);
    ASSERT_EQ_INT(menu.status, DIFFICULTY_MENU_OPEN);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_UP);
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_EASY);
    ASSERT_EQ_INT(menu.status, DIFFICULTY_MENU_OPEN);
}

static void test_apply_difficulty_menu_command_down_moves_towards_hard(void)
{
    DifficultyMenu menu;
    init_difficulty_menu(&menu);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_DOWN);
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_HARD);
    ASSERT_EQ_INT(menu.status, DIFFICULTY_MENU_OPEN);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_DOWN);
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_HARD);
    ASSERT_EQ_INT(menu.status, DIFFICULTY_MENU_OPEN);
}

static void test_apply_difficulty_menu_command_up_and_down_do_not_wrap(void)
{
    DifficultyMenu menu;
    init_difficulty_menu(&menu);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_UP);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_UP);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_UP);
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_EASY);
    for (int step = 0; step < DIFFICULTY_COUNT + 1; step++)
    {
        apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_DOWN);
    }
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_HARD);
}

static void test_apply_difficulty_menu_command_walks_every_level(void)
{
    DifficultyMenu menu;
    init_difficulty_menu(&menu);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_UP);
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_EASY);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_DOWN);
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_MEDIUM);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_DOWN);
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_HARD);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_UP);
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_MEDIUM);
}

static void test_apply_difficulty_menu_command_confirm_keeps_selection(void)
{
    const DifficultyMenuCommand routes[] = {DIFFICULTY_COMMAND_UP,
                                            DIFFICULTY_COMMAND_NONE,
                                            DIFFICULTY_COMMAND_DOWN};
    const Difficulty expected[]          = {DIFFICULTY_EASY, DIFFICULTY_MEDIUM,
                                            DIFFICULTY_HARD};
    for (size_t i = 0; i < sizeof routes / sizeof routes[0]; i++)
    {
        DifficultyMenu menu;
        init_difficulty_menu(&menu);
        apply_difficulty_menu_command(&menu, routes[i]);
        apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_CONFIRM);
        ASSERT_EQ_INT(menu.status, DIFFICULTY_MENU_CONFIRMED);
        ASSERT_EQ_INT(menu.selected, expected[i]);
    }
}

static void test_apply_difficulty_menu_command_back_cancels(void)
{
    DifficultyMenu menu;
    init_difficulty_menu(&menu);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_DOWN);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_BACK);
    ASSERT_EQ_INT(menu.status, DIFFICULTY_MENU_CANCELLED);
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_HARD);
}

static void test_apply_difficulty_menu_command_none_changes_nothing(void)
{
    DifficultyMenu menu;
    init_difficulty_menu(&menu);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_NONE);
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_MEDIUM);
    ASSERT_EQ_INT(menu.status, DIFFICULTY_MENU_OPEN);
}

static void expect_menu_frozen_after(DifficultyMenuCommand closing_command,
                                     DifficultyMenuStatus closed_status)
{
    for (int c = 0; c < TEST_COMMAND_COUNT; c++)
    {
        DifficultyMenu menu;
        init_difficulty_menu(&menu);
        apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_DOWN);
        apply_difficulty_menu_command(&menu, closing_command);
        apply_difficulty_menu_command(&menu, all_commands[c]);
        ASSERT_EQ_INT(menu.status, closed_status);
        ASSERT_EQ_INT(menu.selected, DIFFICULTY_HARD);
    }
}

static void test_apply_difficulty_menu_command_ignored_after_confirm(void)
{
    expect_menu_frozen_after(DIFFICULTY_COMMAND_CONFIRM,
                             DIFFICULTY_MENU_CONFIRMED);
}

static void test_apply_difficulty_menu_command_ignored_after_cancel(void)
{
    expect_menu_frozen_after(DIFFICULTY_COMMAND_BACK,
                             DIFFICULTY_MENU_CANCELLED);
}

static void test_init_difficulty_menu_after_hard_game_selects_medium_again(void)
{
    DifficultyMenu menu;
    init_difficulty_menu(&menu);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_DOWN);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_CONFIRM);
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_HARD);
    ASSERT_EQ_INT(menu.status, DIFFICULTY_MENU_CONFIRMED);
    init_difficulty_menu(&menu);
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_MEDIUM);
    ASSERT_EQ_INT(menu.status, DIFFICULTY_MENU_OPEN);
}

static void test_init_difficulty_menu_after_cancel_reopens_the_menu(void)
{
    DifficultyMenu menu;
    init_difficulty_menu(&menu);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_UP);
    apply_difficulty_menu_command(&menu, DIFFICULTY_COMMAND_BACK);
    init_difficulty_menu(&menu);
    ASSERT_EQ_INT(menu.selected, DIFFICULTY_MEDIUM);
    ASSERT_EQ_INT(menu.status, DIFFICULTY_MENU_OPEN);
}

static void test_difficulty_menu_label_names_each_level(void)
{
    ASSERT_EQ_INT(strcmp(difficulty_menu_label(DIFFICULTY_EASY), "Easy"), 0);
    ASSERT_EQ_INT(strcmp(difficulty_menu_label(DIFFICULTY_MEDIUM), "Medium"),
                  0);
    ASSERT_EQ_INT(strcmp(difficulty_menu_label(DIFFICULTY_HARD), "Hard"), 0);
}

static void test_difficulty_menu_label_clamps_out_of_range(void)
{
    ASSERT_EQ_INT(strcmp(difficulty_menu_label(TEST_BELOW_RANGE), "Easy"), 0);
    ASSERT_EQ_INT(strcmp(difficulty_menu_label(TEST_ABOVE_RANGE), "Hard"), 0);
}

int main(void)
{
    RUN_TEST(test_init_difficulty_menu_selects_medium_and_is_open);
    RUN_TEST(test_apply_difficulty_menu_command_up_moves_towards_easy);
    RUN_TEST(test_apply_difficulty_menu_command_down_moves_towards_hard);
    RUN_TEST(test_apply_difficulty_menu_command_up_and_down_do_not_wrap);
    RUN_TEST(test_apply_difficulty_menu_command_walks_every_level);
    RUN_TEST(test_apply_difficulty_menu_command_confirm_keeps_selection);
    RUN_TEST(test_apply_difficulty_menu_command_back_cancels);
    RUN_TEST(test_apply_difficulty_menu_command_none_changes_nothing);
    RUN_TEST(test_apply_difficulty_menu_command_ignored_after_confirm);
    RUN_TEST(test_apply_difficulty_menu_command_ignored_after_cancel);
    RUN_TEST(test_init_difficulty_menu_after_hard_game_selects_medium_again);
    RUN_TEST(test_init_difficulty_menu_after_cancel_reopens_the_menu);
    RUN_TEST(test_difficulty_menu_label_names_each_level);
    RUN_TEST(test_difficulty_menu_label_clamps_out_of_range);
    return test_harness_result();
}
