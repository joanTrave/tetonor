#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/ui/pause_menu.h"

#define TEST_BELOW_RANGE (-1)
#define TEST_ABOVE_RANGE 7
#define TEST_PAUSE_COMMAND_COUNT 5
#define TEST_PROMPT_COMMAND_COUNT 6

static const PauseMenuCommand all_pause_commands[TEST_PAUSE_COMMAND_COUNT] = {
    PAUSE_MENU_COMMAND_NONE, PAUSE_MENU_COMMAND_UP, PAUSE_MENU_COMMAND_DOWN,
    PAUSE_MENU_COMMAND_SELECT, PAUSE_MENU_COMMAND_RESUME};

static const PromptCommand all_prompt_commands[TEST_PROMPT_COMMAND_COUNT] = {
    PROMPT_COMMAND_NONE,   PROMPT_COMMAND_UP,  PROMPT_COMMAND_DOWN,
    PROMPT_COMMAND_SELECT, PROMPT_COMMAND_YES, PROMPT_COMMAND_NO};

static bool states_equal(const PauseState* first, const PauseState* second)
{
    return first->screen == second->screen &&
           first->selected == second->selected &&
           first->answer == second->answer &&
           first->pending == second->pending &&
           first->cancel_to == second->cancel_to;
}

static void open_menu(PauseState* state)
{
    init_pause_state(state);
    open_pause_menu(state);
}

static void open_prompt_from_game(PauseState* state, GameAction action)
{
    init_pause_state(state);
    request_game_action(state, action, true);
}

static void open_prompt_from_abandon(PauseState* state)
{
    open_menu(state);
    apply_pause_menu_command(state, PAUSE_MENU_COMMAND_DOWN, true);
    apply_pause_menu_command(state, PAUSE_MENU_COMMAND_SELECT, true);
}

static void test_init_pause_state_is_playing(void)
{
    PauseState state;
    memset(&state, 0x5a, sizeof state);
    init_pause_state(&state);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_EQ_INT(state.selected, PAUSE_OPTION_RESUME);
    ASSERT_EQ_INT(state.answer, PROMPT_OPTION_NO);
    ASSERT_EQ_INT(state.pending, GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.cancel_to, PAUSE_SCREEN_PLAYING);
    ASSERT_FALSE(is_game_paused(&state));
}

static void test_open_pause_menu_selects_resume(void)
{
    PauseState state;
    init_pause_state(&state);
    open_pause_menu(&state);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_MENU);
    ASSERT_EQ_INT(state.selected, PAUSE_OPTION_RESUME);
}

static void test_open_pause_menu_selects_resume_every_time(void)
{
    PauseState state;
    open_menu(&state);
    apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_DOWN, false);
    ASSERT_EQ_INT(state.selected, PAUSE_OPTION_ABANDON);
    apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_RESUME, false);
    open_pause_menu(&state);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_MENU);
    ASSERT_EQ_INT(state.selected, PAUSE_OPTION_RESUME);
}

static void test_open_pause_menu_is_ignored_while_paused(void)
{
    PauseState state;
    open_menu(&state);
    apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_DOWN, false);
    PauseState before = state;
    open_pause_menu(&state);
    ASSERT_TRUE(states_equal(&state, &before));
    ASSERT_EQ_INT(state.selected, PAUSE_OPTION_ABANDON);

    open_prompt_from_game(&state, GAME_ACTION_NEW_GAME);
    before = state;
    open_pause_menu(&state);
    ASSERT_TRUE(states_equal(&state, &before));
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PROMPT);
}

static void test_is_game_paused_matches_screen(void)
{
    PauseState state;
    init_pause_state(&state);
    ASSERT_FALSE(is_game_paused(&state));
    open_pause_menu(&state);
    ASSERT_TRUE(is_game_paused(&state));
    apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_RESUME, false);
    ASSERT_FALSE(is_game_paused(&state));
    request_game_action(&state, GAME_ACTION_NEW_GAME, true);
    ASSERT_TRUE(is_game_paused(&state));
    apply_prompt_command(&state, PROMPT_COMMAND_NO);
    ASSERT_FALSE(is_game_paused(&state));
    open_prompt_from_abandon(&state);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PROMPT);
    ASSERT_TRUE(is_game_paused(&state));
}

static void test_apply_pause_menu_command_moves_and_stops_at_ends(void)
{
    PauseState state;
    open_menu(&state);
    ASSERT_EQ_INT(
        apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_UP, false),
        GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.selected, PAUSE_OPTION_RESUME);
    ASSERT_EQ_INT(
        apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_DOWN, false),
        GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.selected, PAUSE_OPTION_ABANDON);
    apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_DOWN, false);
    ASSERT_EQ_INT(state.selected, PAUSE_OPTION_ABANDON);
    apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_UP, false);
    ASSERT_EQ_INT(state.selected, PAUSE_OPTION_RESUME);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_MENU);
}

static void test_apply_pause_menu_command_resume_returns_to_game(void)
{
    PauseState state;
    open_menu(&state);
    ASSERT_EQ_INT(
        apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_RESUME, true),
        GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PLAYING);

    open_menu(&state);
    apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_DOWN, true);
    ASSERT_EQ_INT(state.selected, PAUSE_OPTION_ABANDON);
    ASSERT_EQ_INT(
        apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_RESUME, true),
        GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PLAYING);
}

static void test_apply_pause_menu_command_select_resume(void)
{
    PauseState state;
    open_menu(&state);
    ASSERT_EQ_INT(
        apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_SELECT, true),
        GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_EQ_INT(state.pending, GAME_ACTION_NONE);
}

static void test_apply_pause_menu_command_abandon_without_risk_leaves(void)
{
    PauseState state;
    open_menu(&state);
    apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_DOWN, false);
    ASSERT_EQ_INT(
        apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_SELECT, false),
        GAME_ACTION_MAIN_MENU);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_EQ_INT(state.pending, GAME_ACTION_NONE);
}

static void test_apply_pause_menu_command_abandon_with_risk_opens_prompt(void)
{
    PauseState state;
    open_menu(&state);
    apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_DOWN, true);
    ASSERT_EQ_INT(
        apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_SELECT, true),
        GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PROMPT);
    ASSERT_EQ_INT(state.pending, GAME_ACTION_MAIN_MENU);
    ASSERT_EQ_INT(state.answer, PROMPT_OPTION_NO);
    ASSERT_EQ_INT(state.cancel_to, PAUSE_SCREEN_MENU);
}

static void test_apply_pause_menu_command_none_changes_nothing(void)
{
    PauseState state;
    open_menu(&state);
    apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_DOWN, true);
    PauseState before = state;
    ASSERT_EQ_INT(
        apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_NONE, true),
        GAME_ACTION_NONE);
    ASSERT_TRUE(states_equal(&state, &before));
}

static void test_apply_pause_menu_command_ignored_outside_menu(void)
{
    PauseState playing;
    init_pause_state(&playing);
    PauseState prompt;
    open_prompt_from_abandon(&prompt);
    for (size_t i = 0; i < TEST_PAUSE_COMMAND_COUNT; i++)
    {
        PauseState state = playing;
        ASSERT_EQ_INT(
            apply_pause_menu_command(&state, all_pause_commands[i], true),
            GAME_ACTION_NONE);
        ASSERT_TRUE(states_equal(&state, &playing));

        state = prompt;
        ASSERT_EQ_INT(
            apply_pause_menu_command(&state, all_pause_commands[i], false),
            GAME_ACTION_NONE);
        ASSERT_TRUE(states_equal(&state, &prompt));
    }
}

static void test_request_game_action_without_risk_returns_action(void)
{
    PauseState state;
    init_pause_state(&state);
    PauseState before = state;
    ASSERT_EQ_INT(request_game_action(&state, GAME_ACTION_NEW_GAME, false),
                  GAME_ACTION_NEW_GAME);
    ASSERT_TRUE(states_equal(&state, &before));
    ASSERT_EQ_INT(request_game_action(&state, GAME_ACTION_MAIN_MENU, false),
                  GAME_ACTION_MAIN_MENU);
    ASSERT_TRUE(states_equal(&state, &before));
}

static void test_request_game_action_with_risk_opens_prompt(void)
{
    const GameAction actions[] = {GAME_ACTION_NEW_GAME, GAME_ACTION_MAIN_MENU};
    for (size_t i = 0; i < sizeof actions / sizeof actions[0]; i++)
    {
        PauseState state;
        init_pause_state(&state);
        ASSERT_EQ_INT(request_game_action(&state, actions[i], true),
                      GAME_ACTION_NONE);
        ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PROMPT);
        ASSERT_EQ_INT(state.pending, actions[i]);
        ASSERT_EQ_INT(state.answer, PROMPT_OPTION_NO);
        ASSERT_EQ_INT(state.cancel_to, PAUSE_SCREEN_PLAYING);
    }
}

static void test_request_game_action_ignored_while_paused_or_none(void)
{
    PauseState state;
    init_pause_state(&state);
    PauseState before = state;
    ASSERT_EQ_INT(request_game_action(&state, GAME_ACTION_NONE, true),
                  GAME_ACTION_NONE);
    ASSERT_TRUE(states_equal(&state, &before));
    ASSERT_EQ_INT(request_game_action(&state, GAME_ACTION_NONE, false),
                  GAME_ACTION_NONE);
    ASSERT_TRUE(states_equal(&state, &before));

    open_menu(&state);
    before = state;
    ASSERT_EQ_INT(request_game_action(&state, GAME_ACTION_NEW_GAME, true),
                  GAME_ACTION_NONE);
    ASSERT_TRUE(states_equal(&state, &before));
    ASSERT_EQ_INT(request_game_action(&state, GAME_ACTION_NEW_GAME, false),
                  GAME_ACTION_NONE);
    ASSERT_TRUE(states_equal(&state, &before));

    open_prompt_from_game(&state, GAME_ACTION_NEW_GAME);
    before = state;
    ASSERT_EQ_INT(request_game_action(&state, GAME_ACTION_MAIN_MENU, true),
                  GAME_ACTION_NONE);
    ASSERT_TRUE(states_equal(&state, &before));
    ASSERT_EQ_INT(request_game_action(&state, GAME_ACTION_MAIN_MENU, false),
                  GAME_ACTION_NONE);
    ASSERT_TRUE(states_equal(&state, &before));
    ASSERT_EQ_INT(state.pending, GAME_ACTION_NEW_GAME);
}

static void test_apply_prompt_command_yes_returns_pending(void)
{
    const GameAction actions[] = {GAME_ACTION_NEW_GAME, GAME_ACTION_MAIN_MENU};
    for (size_t i = 0; i < sizeof actions / sizeof actions[0]; i++)
    {
        PauseState state;
        open_prompt_from_game(&state, actions[i]);
        ASSERT_EQ_INT(apply_prompt_command(&state, PROMPT_COMMAND_YES),
                      actions[i]);
        ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PLAYING);
        ASSERT_EQ_INT(state.pending, GAME_ACTION_NONE);
        ASSERT_FALSE(is_game_paused(&state));
    }
}

static void test_apply_prompt_command_yes_from_abandon_returns_main_menu(void)
{
    PauseState state;
    open_prompt_from_abandon(&state);
    ASSERT_EQ_INT(apply_prompt_command(&state, PROMPT_COMMAND_YES),
                  GAME_ACTION_MAIN_MENU);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_EQ_INT(state.pending, GAME_ACTION_NONE);
}

static void test_apply_prompt_command_no_returns_to_game(void)
{
    PauseState state;
    open_prompt_from_game(&state, GAME_ACTION_NEW_GAME);
    ASSERT_EQ_INT(apply_prompt_command(&state, PROMPT_COMMAND_NO),
                  GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_EQ_INT(state.pending, GAME_ACTION_NONE);
}

static void test_apply_prompt_command_no_returns_to_menu_on_abandon(void)
{
    PauseState state;
    open_prompt_from_abandon(&state);
    ASSERT_EQ_INT(apply_prompt_command(&state, PROMPT_COMMAND_NO),
                  GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_MENU);
    ASSERT_EQ_INT(state.selected, PAUSE_OPTION_ABANDON);
    ASSERT_EQ_INT(state.pending, GAME_ACTION_NONE);
    ASSERT_TRUE(is_game_paused(&state));
}

static void test_apply_prompt_command_moves_and_stops_at_ends(void)
{
    PauseState state;
    open_prompt_from_game(&state, GAME_ACTION_NEW_GAME);
    ASSERT_EQ_INT(state.answer, PROMPT_OPTION_NO);
    ASSERT_EQ_INT(apply_prompt_command(&state, PROMPT_COMMAND_DOWN),
                  GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.answer, PROMPT_OPTION_NO);
    ASSERT_EQ_INT(apply_prompt_command(&state, PROMPT_COMMAND_UP),
                  GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.answer, PROMPT_OPTION_YES);
    apply_prompt_command(&state, PROMPT_COMMAND_UP);
    ASSERT_EQ_INT(state.answer, PROMPT_OPTION_YES);
    apply_prompt_command(&state, PROMPT_COMMAND_DOWN);
    ASSERT_EQ_INT(state.answer, PROMPT_OPTION_NO);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PROMPT);
    ASSERT_EQ_INT(state.pending, GAME_ACTION_NEW_GAME);
}

static void test_apply_prompt_command_select_uses_the_answer(void)
{
    PauseState state;
    open_prompt_from_game(&state, GAME_ACTION_NEW_GAME);
    ASSERT_EQ_INT(apply_prompt_command(&state, PROMPT_COMMAND_SELECT),
                  GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PLAYING);

    open_prompt_from_game(&state, GAME_ACTION_NEW_GAME);
    apply_prompt_command(&state, PROMPT_COMMAND_UP);
    ASSERT_EQ_INT(apply_prompt_command(&state, PROMPT_COMMAND_SELECT),
                  GAME_ACTION_NEW_GAME);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_EQ_INT(state.pending, GAME_ACTION_NONE);

    open_prompt_from_game(&state, GAME_ACTION_MAIN_MENU);
    apply_prompt_command(&state, PROMPT_COMMAND_UP);
    apply_prompt_command(&state, PROMPT_COMMAND_DOWN);
    ASSERT_EQ_INT(apply_prompt_command(&state, PROMPT_COMMAND_SELECT),
                  GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PLAYING);
}

static void test_apply_prompt_command_select_no_on_abandon_returns_to_menu(void)
{
    PauseState state;
    open_prompt_from_abandon(&state);
    ASSERT_EQ_INT(apply_prompt_command(&state, PROMPT_COMMAND_SELECT),
                  GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_MENU);
    ASSERT_EQ_INT(state.selected, PAUSE_OPTION_ABANDON);
}

static void test_apply_prompt_command_none_changes_nothing(void)
{
    PauseState state;
    open_prompt_from_game(&state, GAME_ACTION_MAIN_MENU);
    apply_prompt_command(&state, PROMPT_COMMAND_UP);
    PauseState before = state;
    ASSERT_EQ_INT(apply_prompt_command(&state, PROMPT_COMMAND_NONE),
                  GAME_ACTION_NONE);
    ASSERT_TRUE(states_equal(&state, &before));
}

static void test_apply_prompt_command_ignored_outside_prompt(void)
{
    PauseState playing;
    init_pause_state(&playing);
    PauseState menu;
    open_menu(&menu);
    apply_pause_menu_command(&menu, PAUSE_MENU_COMMAND_DOWN, false);
    for (size_t i = 0; i < TEST_PROMPT_COMMAND_COUNT; i++)
    {
        PauseState state = playing;
        ASSERT_EQ_INT(apply_prompt_command(&state, all_prompt_commands[i]),
                      GAME_ACTION_NONE);
        ASSERT_TRUE(states_equal(&state, &playing));

        state = menu;
        ASSERT_EQ_INT(apply_prompt_command(&state, all_prompt_commands[i]),
                      GAME_ACTION_NONE);
        ASSERT_TRUE(states_equal(&state, &menu));
    }
}

static void test_prompt_reopens_with_no_selected(void)
{
    PauseState state;
    open_prompt_from_game(&state, GAME_ACTION_NEW_GAME);
    apply_prompt_command(&state, PROMPT_COMMAND_UP);
    ASSERT_EQ_INT(state.answer, PROMPT_OPTION_YES);
    apply_prompt_command(&state, PROMPT_COMMAND_NO);
    request_game_action(&state, GAME_ACTION_NEW_GAME, true);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PROMPT);
    ASSERT_EQ_INT(state.answer, PROMPT_OPTION_NO);

    open_prompt_from_abandon(&state);
    apply_prompt_command(&state, PROMPT_COMMAND_UP);
    apply_prompt_command(&state, PROMPT_COMMAND_NO);
    apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_SELECT, true);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PROMPT);
    ASSERT_EQ_INT(state.answer, PROMPT_OPTION_NO);
}

static void test_pause_state_never_returns_an_action_while_paused(void)
{
    for (size_t command = 0; command < TEST_PAUSE_COMMAND_COUNT; command++)
    {
        for (int risk = 0; risk <= 1; risk++)
        {
            PauseState state;
            open_menu(&state);
            apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_DOWN, risk);
            GameAction action = apply_pause_menu_command(
                &state, all_pause_commands[command], risk);
            if (action != GAME_ACTION_NONE)
            {
                ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PLAYING);
            }
            ASSERT_EQ_INT(is_game_paused(&state),
                          state.screen != PAUSE_SCREEN_PLAYING);
        }
    }
    for (size_t command = 0; command < TEST_PROMPT_COMMAND_COUNT; command++)
    {
        PauseState state;
        open_prompt_from_abandon(&state);
        GameAction action =
            apply_prompt_command(&state, all_prompt_commands[command]);
        if (action != GAME_ACTION_NONE)
        {
            ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PLAYING);
        }
        ASSERT_EQ_INT(is_game_paused(&state),
                      state.screen != PAUSE_SCREEN_PLAYING);
    }
}

static void test_pause_labels_and_questions(void)
{
    ASSERT_TRUE(strcmp(pause_option_label(PAUSE_OPTION_RESUME), "Resume") == 0);
    ASSERT_TRUE(strcmp(pause_option_label(PAUSE_OPTION_ABANDON), "Abandon") ==
                0);
    ASSERT_TRUE(strcmp(pause_option_label(TEST_BELOW_RANGE), "Resume") == 0);
    ASSERT_TRUE(strcmp(pause_option_label(TEST_ABOVE_RANGE), "Abandon") == 0);
    ASSERT_TRUE(strcmp(prompt_option_label(PROMPT_OPTION_YES), "Yes") == 0);
    ASSERT_TRUE(strcmp(prompt_option_label(PROMPT_OPTION_NO), "No") == 0);
    ASSERT_TRUE(strcmp(prompt_option_label(TEST_BELOW_RANGE), "Yes") == 0);
    ASSERT_TRUE(strcmp(prompt_option_label(TEST_ABOVE_RANGE), "No") == 0);
    ASSERT_TRUE(strcmp(prompt_question(GAME_ACTION_NEW_GAME),
                       "Start a new game?") == 0);
    ASSERT_TRUE(strcmp(prompt_question(GAME_ACTION_MAIN_MENU),
                       "Abandon this game?") == 0);
    ASSERT_TRUE(strcmp(prompt_question(GAME_ACTION_NONE), "") == 0);
    ASSERT_TRUE(strcmp(prompt_question(TEST_BELOW_RANGE), "") == 0);
    ASSERT_TRUE(strcmp(prompt_question(TEST_ABOVE_RANGE), "") == 0);
}

#define TEST_SAVE_NOTICE_ABOVE_RANGE 99

static void test_game_action_save_is_a_new_action_after_the_existing_ones(void)
{
    ASSERT_EQ_INT(GAME_ACTION_NONE, 0);
    ASSERT_EQ_INT(GAME_ACTION_NEW_GAME, 1);
    ASSERT_EQ_INT(GAME_ACTION_MAIN_MENU, 2);
    ASSERT_EQ_INT(GAME_ACTION_SAVE, 3);
}

static void test_pause_options_are_still_resume_and_abandon(void)
{
    ASSERT_EQ_INT(PAUSE_OPTION_RESUME, 0);
    ASSERT_EQ_INT(PAUSE_OPTION_ABANDON, 1);
    ASSERT_EQ_INT(PAUSE_OPTION_COUNT, 2);
}

static void test_init_pause_state_has_no_save_notice(void)
{
    PauseState state;
    memset(&state, 0x5a, sizeof state);
    init_pause_state(&state);
    ASSERT_EQ_INT(state.save_notice, PAUSE_SAVE_NOTICE_NONE);
}

static void test_open_pause_menu_clears_the_save_notice(void)
{
    PauseState state;
    open_menu(&state);
    set_pause_save_notice(&state, PAUSE_SAVE_NOTICE_SAVED);
    apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_RESUME, false);
    ASSERT_EQ_INT(state.save_notice, PAUSE_SAVE_NOTICE_SAVED);
    open_pause_menu(&state);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_MENU);
    ASSERT_EQ_INT(state.save_notice, PAUSE_SAVE_NOTICE_NONE);
}

static void
test_open_pause_menu_ignored_while_paused_keeps_the_save_notice(void)
{
    PauseState state;
    open_menu(&state);
    set_pause_save_notice(&state, PAUSE_SAVE_NOTICE_FAILED);
    open_pause_menu(&state);
    ASSERT_EQ_INT(state.save_notice, PAUSE_SAVE_NOTICE_FAILED);
}

static void test_set_pause_save_notice_sets_every_notice(void)
{
    for (int notice = 0; notice < PAUSE_SAVE_NOTICE_COUNT; notice++)
    {
        PauseState state;
        open_menu(&state);
        apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_DOWN, false);
        PauseState before = state;
        set_pause_save_notice(&state, (PauseSaveNotice)notice);
        ASSERT_EQ_INT(state.save_notice, notice);
        ASSERT_TRUE(states_equal(&state, &before));
    }
}

static void test_set_pause_save_notice_replaces_the_previous_notice(void)
{
    PauseState state;
    open_menu(&state);
    set_pause_save_notice(&state, PAUSE_SAVE_NOTICE_FAILED);
    set_pause_save_notice(&state, PAUSE_SAVE_NOTICE_SAVED);
    ASSERT_EQ_INT(state.save_notice, PAUSE_SAVE_NOTICE_SAVED);
    set_pause_save_notice(&state, PAUSE_SAVE_NOTICE_NONE);
    ASSERT_EQ_INT(state.save_notice, PAUSE_SAVE_NOTICE_NONE);
}

static void test_apply_pause_menu_command_save_asks_for_a_save(void)
{
    PauseState state;
    open_menu(&state);
    ASSERT_EQ_INT(
        apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_SAVE, false),
        GAME_ACTION_SAVE);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_MENU);
    ASSERT_EQ_INT(state.selected, PAUSE_OPTION_RESUME);
    ASSERT_EQ_INT(state.pending, GAME_ACTION_NONE);
    ASSERT_TRUE(is_game_paused(&state));
}

static void test_apply_pause_menu_command_save_keeps_the_selection(void)
{
    PauseState state;
    open_menu(&state);
    apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_DOWN, false);
    PauseState before = state;
    ASSERT_EQ_INT(
        apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_SAVE, false),
        GAME_ACTION_SAVE);
    ASSERT_TRUE(states_equal(&state, &before));
    ASSERT_EQ_INT(state.selected, PAUSE_OPTION_ABANDON);
}

static void test_apply_pause_menu_command_save_never_opens_a_prompt(void)
{
    PauseState state;
    open_menu(&state);
    ASSERT_EQ_INT(
        apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_SAVE, true),
        GAME_ACTION_SAVE);
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_MENU);
    ASSERT_EQ_INT(state.pending, GAME_ACTION_NONE);
}

static void test_apply_pause_menu_command_save_can_repeat(void)
{
    PauseState state;
    open_menu(&state);
    for (int i = 0; i < 3; i++)
    {
        ASSERT_EQ_INT(
            apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_SAVE, false),
            GAME_ACTION_SAVE);
    }
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_MENU);
}

static void test_apply_pause_menu_command_save_does_not_set_a_notice(void)
{
    PauseState state;
    open_menu(&state);
    apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_SAVE, false);
    ASSERT_EQ_INT(state.save_notice, PAUSE_SAVE_NOTICE_NONE);
}

static void test_apply_pause_menu_command_save_ignored_while_playing(void)
{
    PauseState state;
    init_pause_state(&state);
    PauseState before = state;
    ASSERT_EQ_INT(
        apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_SAVE, false),
        GAME_ACTION_NONE);
    ASSERT_TRUE(states_equal(&state, &before));
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PLAYING);
}

static void test_apply_pause_menu_command_save_ignored_in_a_prompt(void)
{
    PauseState state;
    open_prompt_from_game(&state, GAME_ACTION_NEW_GAME);
    PauseState before = state;
    ASSERT_EQ_INT(
        apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_SAVE, true),
        GAME_ACTION_NONE);
    ASSERT_TRUE(states_equal(&state, &before));
    open_prompt_from_abandon(&state);
    before = state;
    ASSERT_EQ_INT(
        apply_pause_menu_command(&state, PAUSE_MENU_COMMAND_SAVE, true),
        GAME_ACTION_NONE);
    ASSERT_TRUE(states_equal(&state, &before));
    ASSERT_EQ_INT(state.screen, PAUSE_SCREEN_PROMPT);
}

static void test_pause_save_notice_text_says_what_happened(void)
{
    ASSERT_TRUE(strcmp(pause_save_notice_text(PAUSE_SAVE_NOTICE_NONE), "") ==
                0);
    ASSERT_TRUE(strcmp(pause_save_notice_text(PAUSE_SAVE_NOTICE_SAVED),
                       "Game saved.") == 0);
    ASSERT_TRUE(strcmp(pause_save_notice_text(PAUSE_SAVE_NOTICE_SESSION_ONLY),
                       "Saved for this session only.") == 0);
    ASSERT_TRUE(strcmp(pause_save_notice_text(PAUSE_SAVE_NOTICE_FAILED),
                       "This game cannot be saved.") == 0);
}

static void test_pause_save_notice_text_clamps_out_of_range_values(void)
{
    ASSERT_TRUE(strcmp(pause_save_notice_text(TEST_BELOW_RANGE), "") == 0);
    ASSERT_TRUE(strcmp(pause_save_notice_text(PAUSE_SAVE_NOTICE_COUNT),
                       pause_save_notice_text(PAUSE_SAVE_NOTICE_FAILED)) == 0);
    ASSERT_TRUE(strcmp(pause_save_notice_text(TEST_SAVE_NOTICE_ABOVE_RANGE),
                       pause_save_notice_text(PAUSE_SAVE_NOTICE_FAILED)) == 0);
}

static void test_pause_save_notice_texts_differ(void)
{
    for (int first = 0; first < PAUSE_SAVE_NOTICE_COUNT; first++)
    {
        for (int second = first + 1; second < PAUSE_SAVE_NOTICE_COUNT; second++)
        {
            ASSERT_TRUE(strcmp(pause_save_notice_text(first),
                               pause_save_notice_text(second)) != 0);
        }
    }
}

static void test_prompt_question_for_save_is_empty(void)
{
    ASSERT_TRUE(strcmp(prompt_question(GAME_ACTION_SAVE), "") == 0);
}

int main(void)
{
    RUN_TEST(test_init_pause_state_is_playing);
    RUN_TEST(test_open_pause_menu_selects_resume);
    RUN_TEST(test_open_pause_menu_selects_resume_every_time);
    RUN_TEST(test_open_pause_menu_is_ignored_while_paused);
    RUN_TEST(test_is_game_paused_matches_screen);
    RUN_TEST(test_apply_pause_menu_command_moves_and_stops_at_ends);
    RUN_TEST(test_apply_pause_menu_command_resume_returns_to_game);
    RUN_TEST(test_apply_pause_menu_command_select_resume);
    RUN_TEST(test_apply_pause_menu_command_abandon_without_risk_leaves);
    RUN_TEST(test_apply_pause_menu_command_abandon_with_risk_opens_prompt);
    RUN_TEST(test_apply_pause_menu_command_none_changes_nothing);
    RUN_TEST(test_apply_pause_menu_command_ignored_outside_menu);
    RUN_TEST(test_request_game_action_without_risk_returns_action);
    RUN_TEST(test_request_game_action_with_risk_opens_prompt);
    RUN_TEST(test_request_game_action_ignored_while_paused_or_none);
    RUN_TEST(test_apply_prompt_command_yes_returns_pending);
    RUN_TEST(test_apply_prompt_command_yes_from_abandon_returns_main_menu);
    RUN_TEST(test_apply_prompt_command_no_returns_to_game);
    RUN_TEST(test_apply_prompt_command_no_returns_to_menu_on_abandon);
    RUN_TEST(test_apply_prompt_command_moves_and_stops_at_ends);
    RUN_TEST(test_apply_prompt_command_select_uses_the_answer);
    RUN_TEST(test_apply_prompt_command_select_no_on_abandon_returns_to_menu);
    RUN_TEST(test_apply_prompt_command_none_changes_nothing);
    RUN_TEST(test_apply_prompt_command_ignored_outside_prompt);
    RUN_TEST(test_prompt_reopens_with_no_selected);
    RUN_TEST(test_pause_state_never_returns_an_action_while_paused);
    RUN_TEST(test_pause_labels_and_questions);
    RUN_TEST(test_game_action_save_is_a_new_action_after_the_existing_ones);
    RUN_TEST(test_pause_options_are_still_resume_and_abandon);
    RUN_TEST(test_init_pause_state_has_no_save_notice);
    RUN_TEST(test_open_pause_menu_clears_the_save_notice);
    RUN_TEST(test_open_pause_menu_ignored_while_paused_keeps_the_save_notice);
    RUN_TEST(test_set_pause_save_notice_sets_every_notice);
    RUN_TEST(test_set_pause_save_notice_replaces_the_previous_notice);
    RUN_TEST(test_apply_pause_menu_command_save_asks_for_a_save);
    RUN_TEST(test_apply_pause_menu_command_save_keeps_the_selection);
    RUN_TEST(test_apply_pause_menu_command_save_never_opens_a_prompt);
    RUN_TEST(test_apply_pause_menu_command_save_can_repeat);
    RUN_TEST(test_apply_pause_menu_command_save_does_not_set_a_notice);
    RUN_TEST(test_apply_pause_menu_command_save_ignored_while_playing);
    RUN_TEST(test_apply_pause_menu_command_save_ignored_in_a_prompt);
    RUN_TEST(test_pause_save_notice_text_says_what_happened);
    RUN_TEST(test_pause_save_notice_text_clamps_out_of_range_values);
    RUN_TEST(test_pause_save_notice_texts_differ);
    RUN_TEST(test_prompt_question_for_save_is_empty);
    return test_harness_result();
}
