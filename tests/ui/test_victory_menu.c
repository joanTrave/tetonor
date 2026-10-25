#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/game/game_port.h"
#include "../../src/ui/victory_menu.h"

#define TEST_BELOW_RANGE (-1)
#define TEST_ABOVE_RANGE 7
#define TEST_COMMAND_COUNT 6
#define TEST_ELAPSED_SECONDS 65
#define TEST_MOVES 42
#define TEST_LINE_BUFFER_SIZE 64
#define TEST_POISON 0x5a

static const VictoryMenuCommand all_commands[TEST_COMMAND_COUNT] = {
    VICTORY_MENU_COMMAND_NONE,     VICTORY_MENU_COMMAND_UP,
    VICTORY_MENU_COMMAND_DOWN,     VICTORY_MENU_COMMAND_SELECT,
    VICTORY_MENU_COMMAND_NEW_GAME, VICTORY_MENU_COMMAND_MAIN_MENU};

/* Line by line: the bytes after a line's terminator are unspecified. */
static bool summaries_equal(const VictorySummary* first,
                            const VictorySummary* second)
{
    for (int line = 0; line < VICTORY_SUMMARY_LINE_COUNT; line++)
    {
        if (strcmp(first->lines[line], second->lines[line]) != 0)
        {
            return false;
        }
    }
    return true;
}

static bool states_equal(const VictoryState* first, const VictoryState* second)
{
    return first->shown == second->shown &&
           first->selected == second->selected &&
           summaries_equal(&first->summary, &second->summary);
}

static void open_default(VictoryState* state)
{
    init_victory_state(state);
    open_victory_screen(state, TEST_ELAPSED_SECONDS, DIFFICULTY_MEDIUM,
                        TEST_MOVES);
}

static void select_main_menu(VictoryState* state)
{
    apply_victory_menu_command(state, VICTORY_MENU_COMMAND_DOWN);
}

/* The label padded to the value column, then the value. */
static void expected_line(char* out, const char* label, const char* value)
{
    snprintf(out, TEST_LINE_BUFFER_SIZE, "%-*s%s", VICTORY_SUMMARY_LABEL_W,
             label, value);
}

static void test_init_victory_state_is_hidden(void)
{
    VictoryState state;
    memset(&state, TEST_POISON, sizeof state);
    init_victory_state(&state);
    ASSERT_FALSE(state.shown);
    ASSERT_FALSE(is_victory_shown(&state));
    ASSERT_EQ_INT(state.selected, VICTORY_OPTION_NEW_GAME);
    for (int line = 0; line < VICTORY_SUMMARY_LINE_COUNT; line++)
    {
        ASSERT_EQ_INT(strlen(state.summary.lines[line]), 0);
    }
}

static void test_is_victory_shown_follows_the_screen(void)
{
    VictoryState state;
    init_victory_state(&state);
    ASSERT_FALSE(is_victory_shown(&state));
    open_victory_screen(&state, 0, DIFFICULTY_EASY, 0);
    ASSERT_TRUE(is_victory_shown(&state));
    init_victory_state(&state);
    ASSERT_FALSE(is_victory_shown(&state));
}

static void test_open_victory_screen_shows_and_selects_new_game(void)
{
    VictoryState state;
    init_victory_state(&state);
    state.selected = VICTORY_OPTION_MAIN_MENU;
    open_victory_screen(&state, TEST_ELAPSED_SECONDS, DIFFICULTY_MEDIUM,
                        TEST_MOVES);
    ASSERT_TRUE(state.shown);
    ASSERT_TRUE(is_victory_shown(&state));
    ASSERT_EQ_INT(state.selected, VICTORY_OPTION_NEW_GAME);
}

static void test_open_victory_screen_builds_the_summary(void)
{
    VictoryState state;
    open_default(&state);
    VictorySummary expected;
    build_victory_summary(TEST_ELAPSED_SECONDS, DIFFICULTY_MEDIUM, TEST_MOVES,
                          &expected);
    ASSERT_TRUE(summaries_equal(&state.summary, &expected));
    ASSERT_EQ_INT(
        strcmp(state.summary.lines[VICTORY_SUMMARY_TIME], "Time:       01:05"),
        0);
    ASSERT_EQ_INT(strcmp(state.summary.lines[VICTORY_SUMMARY_DIFFICULTY],
                         "Difficulty: Medium"),
                  0);
    ASSERT_EQ_INT(
        strcmp(state.summary.lines[VICTORY_SUMMARY_MOVES], "Moves:      42"),
        0);
}

static void test_open_victory_screen_twice_keeps_the_first_summary(void)
{
    VictoryState state;
    open_default(&state);
    select_main_menu(&state);
    VictoryState before = state;
    open_victory_screen(&state, 3600, DIFFICULTY_HARD, 7);
    ASSERT_TRUE(states_equal(&state, &before));
    ASSERT_EQ_INT(state.selected, VICTORY_OPTION_MAIN_MENU);
    ASSERT_EQ_INT(
        strcmp(state.summary.lines[VICTORY_SUMMARY_TIME], "Time:       01:05"),
        0);
}

static void test_init_victory_state_hides_a_shown_screen(void)
{
    VictoryState state;
    open_default(&state);
    select_main_menu(&state);
    init_victory_state(&state);
    ASSERT_FALSE(is_victory_shown(&state));
    ASSERT_EQ_INT(state.selected, VICTORY_OPTION_NEW_GAME);
    ASSERT_EQ_INT(strlen(state.summary.lines[VICTORY_SUMMARY_TIME]), 0);
    open_victory_screen(&state, 3600, DIFFICULTY_HARD, 7);
    ASSERT_TRUE(is_victory_shown(&state));
    ASSERT_EQ_INT(strcmp(state.summary.lines[VICTORY_SUMMARY_TIME],
                         "Time:       1:00:00"),
                  0);
}

static void test_apply_victory_menu_command_moves_and_stops_at_ends(void)
{
    VictoryState state;
    open_default(&state);
    ASSERT_EQ_INT(apply_victory_menu_command(&state, VICTORY_MENU_COMMAND_UP),
                  GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.selected, VICTORY_OPTION_NEW_GAME);
    ASSERT_EQ_INT(apply_victory_menu_command(&state, VICTORY_MENU_COMMAND_DOWN),
                  GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.selected, VICTORY_OPTION_MAIN_MENU);
    ASSERT_EQ_INT(apply_victory_menu_command(&state, VICTORY_MENU_COMMAND_DOWN),
                  GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.selected, VICTORY_OPTION_MAIN_MENU);
    ASSERT_EQ_INT(apply_victory_menu_command(&state, VICTORY_MENU_COMMAND_UP),
                  GAME_ACTION_NONE);
    ASSERT_EQ_INT(state.selected, VICTORY_OPTION_NEW_GAME);
    ASSERT_TRUE(is_victory_shown(&state));
}

static void test_apply_victory_menu_command_select_returns_selected_action(void)
{
    VictoryState state;
    open_default(&state);
    ASSERT_EQ_INT(
        apply_victory_menu_command(&state, VICTORY_MENU_COMMAND_SELECT),
        GAME_ACTION_NEW_GAME);
    ASSERT_EQ_INT(state.selected, VICTORY_OPTION_NEW_GAME);
    select_main_menu(&state);
    ASSERT_EQ_INT(
        apply_victory_menu_command(&state, VICTORY_MENU_COMMAND_SELECT),
        GAME_ACTION_MAIN_MENU);
    ASSERT_EQ_INT(state.selected, VICTORY_OPTION_MAIN_MENU);
}

static void test_apply_victory_menu_command_shortcuts_return_their_action(void)
{
    for (int option = 0; option < VICTORY_OPTION_COUNT; option++)
    {
        VictoryState state;
        open_default(&state);
        state.selected = (VictoryOption)option;
        ASSERT_EQ_INT(
            apply_victory_menu_command(&state, VICTORY_MENU_COMMAND_NEW_GAME),
            GAME_ACTION_NEW_GAME);
        ASSERT_EQ_INT(
            apply_victory_menu_command(&state, VICTORY_MENU_COMMAND_MAIN_MENU),
            GAME_ACTION_MAIN_MENU);
        ASSERT_EQ_INT(state.selected, option);
    }
}

static void test_apply_victory_menu_command_keeps_the_screen_shown(void)
{
    for (size_t i = 0; i < TEST_COMMAND_COUNT; i++)
    {
        VictoryState state;
        open_default(&state);
        VictoryState before = state;
        apply_victory_menu_command(&state, all_commands[i]);
        ASSERT_TRUE(state.shown);
        ASSERT_TRUE(summaries_equal(&state.summary, &before.summary));
    }
}

static void test_apply_victory_menu_command_ignored_when_hidden(void)
{
    for (size_t i = 0; i < TEST_COMMAND_COUNT; i++)
    {
        VictoryState state;
        init_victory_state(&state);
        VictoryState before = state;
        ASSERT_EQ_INT(apply_victory_menu_command(&state, all_commands[i]),
                      GAME_ACTION_NONE);
        ASSERT_TRUE(states_equal(&state, &before));
        ASSERT_FALSE(is_victory_shown(&state));
    }
}

static void test_apply_victory_menu_command_none_changes_nothing(void)
{
    VictoryState state;
    open_default(&state);
    select_main_menu(&state);
    VictoryState before = state;
    ASSERT_EQ_INT(apply_victory_menu_command(&state, VICTORY_MENU_COMMAND_NONE),
                  GAME_ACTION_NONE);
    ASSERT_TRUE(states_equal(&state, &before));
}

static void assert_time_line(int32_t seconds, const char* expected_time)
{
    VictorySummary summary;
    build_victory_summary(seconds, DIFFICULTY_EASY, 0, &summary);
    char timer_text[TETONOR_ELAPSED_TEXT_SIZE];
    ASSERT_TRUE(format_elapsed_time(seconds, timer_text, sizeof timer_text));
    ASSERT_EQ_INT(strcmp(timer_text, expected_time), 0);
    char expected[TEST_LINE_BUFFER_SIZE];
    expected_line(expected, VICTORY_SUMMARY_TIME_LABEL, timer_text);
    ASSERT_EQ_INT(strcmp(summary.lines[VICTORY_SUMMARY_TIME], expected), 0);
}

static void test_build_victory_summary_formats_time_like_the_timer(void)
{
    VictorySummary summary;
    build_victory_summary(0, DIFFICULTY_EASY, 0, &summary);
    ASSERT_EQ_INT(
        strcmp(summary.lines[VICTORY_SUMMARY_TIME], "Time:       00:00"), 0);
    build_victory_summary(TEST_ELAPSED_SECONDS, DIFFICULTY_EASY, 0, &summary);
    ASSERT_EQ_INT(
        strcmp(summary.lines[VICTORY_SUMMARY_TIME], "Time:       01:05"), 0);
    build_victory_summary(3600, DIFFICULTY_EASY, 0, &summary);
    ASSERT_EQ_INT(
        strcmp(summary.lines[VICTORY_SUMMARY_TIME], "Time:       1:00:00"), 0);
    build_victory_summary(TETONOR_TIMER_MAX_SECONDS, DIFFICULTY_EASY, 0,
                          &summary);
    ASSERT_EQ_INT(
        strcmp(summary.lines[VICTORY_SUMMARY_TIME], "Time:       99:59:59"), 0);
    assert_time_line(0, "00:00");
    assert_time_line(59, "00:59");
    assert_time_line(TEST_ELAPSED_SECONDS, "01:05");
    assert_time_line(3599, "59:59");
    assert_time_line(3600, "1:00:00");
    assert_time_line(3661, "1:01:01");
    assert_time_line(TETONOR_TIMER_MAX_SECONDS, "99:59:59");
}

static void test_build_victory_summary_clamps_the_time_like_the_timer(void)
{
    VictorySummary summary;
    build_victory_summary(TETONOR_TIMER_MAX_SECONDS + 1, DIFFICULTY_EASY, 0,
                          &summary);
    ASSERT_EQ_INT(
        strcmp(summary.lines[VICTORY_SUMMARY_TIME], "Time:       99:59:59"), 0);
    build_victory_summary(-5, DIFFICULTY_EASY, 0, &summary);
    ASSERT_EQ_INT(
        strcmp(summary.lines[VICTORY_SUMMARY_TIME], "Time:       00:00"), 0);
}

static void test_build_victory_summary_names_the_difficulty(void)
{
    const struct
    {
        int difficulty;
        const char* line;
    } cases[] = {{DIFFICULTY_EASY, "Difficulty: Easy"},
                 {DIFFICULTY_MEDIUM, "Difficulty: Medium"},
                 {DIFFICULTY_HARD, "Difficulty: Hard"},
                 {TEST_BELOW_RANGE, "Difficulty: Easy"},
                 {TEST_ABOVE_RANGE, "Difficulty: Hard"}};
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++)
    {
        VictorySummary summary;
        build_victory_summary(0, cases[i].difficulty, 0, &summary);
        ASSERT_EQ_INT(
            strcmp(summary.lines[VICTORY_SUMMARY_DIFFICULTY], cases[i].line),
            0);
    }
}

static void test_build_victory_summary_counts_moves(void)
{
    const struct
    {
        int moves;
        const char* line;
    } cases[] = {
        {0, "Moves:      0"},           {1, "Moves:      1"},
        {TEST_MOVES, "Moves:      42"}, {INT_MAX, "Moves:      2147483647"},
        {-1, "Moves:      0"},          {INT_MIN, "Moves:      0"}};
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++)
    {
        VictorySummary summary;
        build_victory_summary(0, DIFFICULTY_EASY, cases[i].moves, &summary);
        ASSERT_EQ_INT(
            strcmp(summary.lines[VICTORY_SUMMARY_MOVES], cases[i].line), 0);
    }
}

static void test_build_victory_summary_overwrites_previous_content(void)
{
    VictorySummary summary;
    memset(&summary, TEST_POISON, sizeof summary);
    build_victory_summary(TEST_ELAPSED_SECONDS, DIFFICULTY_HARD, TEST_MOVES,
                          &summary);
    ASSERT_EQ_INT(
        strcmp(summary.lines[VICTORY_SUMMARY_TIME], "Time:       01:05"), 0);
    ASSERT_EQ_INT(
        strcmp(summary.lines[VICTORY_SUMMARY_DIFFICULTY], "Difficulty: Hard"),
        0);
    ASSERT_EQ_INT(
        strcmp(summary.lines[VICTORY_SUMMARY_MOVES], "Moves:      42"), 0);
}

static void
assert_values_start_at_the_label_width(const VictorySummary* summary)
{
    const char* labels[VICTORY_SUMMARY_LINE_COUNT] = {
        VICTORY_SUMMARY_TIME_LABEL, VICTORY_SUMMARY_DIFFICULTY_LABEL,
        VICTORY_SUMMARY_MOVES_LABEL};
    for (int line = 0; line < VICTORY_SUMMARY_LINE_COUNT; line++)
    {
        const char* text = summary->lines[line];
        size_t label_len = strlen(labels[line]);
        ASSERT_TRUE(strlen(text) < VICTORY_SUMMARY_LINE_SIZE);
        ASSERT_TRUE(strlen(text) > VICTORY_SUMMARY_LABEL_W);
        ASSERT_TRUE(label_len < VICTORY_SUMMARY_LABEL_W);
        ASSERT_EQ_INT(strncmp(text, labels[line], label_len), 0);
        for (size_t col = label_len; col < VICTORY_SUMMARY_LABEL_W; col++)
        {
            ASSERT_EQ_INT(text[col], ' ');
        }
        ASSERT_TRUE(text[VICTORY_SUMMARY_LABEL_W] != ' ');
    }
}

static void test_build_victory_summary_lines_fit(void)
{
    const int32_t times[]    = {0, TEST_ELAPSED_SECONDS, 3600,
                                TETONOR_TIMER_MAX_SECONDS};
    const int difficulties[] = {DIFFICULTY_EASY, DIFFICULTY_MEDIUM,
                                DIFFICULTY_HARD};
    const int moves[]        = {0, TEST_MOVES, INT_MAX};
    for (size_t t = 0; t < sizeof times / sizeof times[0]; t++)
    {
        for (size_t d = 0; d < sizeof difficulties / sizeof difficulties[0];
             d++)
        {
            for (size_t m = 0; m < sizeof moves / sizeof moves[0]; m++)
            {
                VictorySummary summary;
                build_victory_summary(times[t], difficulties[d], moves[m],
                                      &summary);
                assert_values_start_at_the_label_width(&summary);
            }
        }
    }
}

static void test_victory_summary_constants_hold_the_longest_line(void)
{
    ASSERT_EQ_INT(VICTORY_SUMMARY_LINE_COUNT, 3);
    ASSERT_EQ_INT(VICTORY_OPTION_COUNT, 2);
    ASSERT_EQ_INT(VICTORY_SUMMARY_LABEL_W, 12);
    ASSERT_TRUE(VICTORY_SUMMARY_LINE_SIZE >= VICTORY_SUMMARY_LABEL_W + 10 + 1);
    ASSERT_EQ_INT(strcmp(VICTORY_SUMMARY_TIME_LABEL, "Time:"), 0);
    ASSERT_EQ_INT(strcmp(VICTORY_SUMMARY_DIFFICULTY_LABEL, "Difficulty:"), 0);
    ASSERT_EQ_INT(strcmp(VICTORY_SUMMARY_MOVES_LABEL, "Moves:"), 0);
}

static void test_victory_option_labels(void)
{
    ASSERT_EQ_INT(
        strcmp(victory_option_label(VICTORY_OPTION_NEW_GAME), "New game"), 0);
    ASSERT_EQ_INT(
        strcmp(victory_option_label(VICTORY_OPTION_MAIN_MENU), "Main menu"), 0);
}

static void test_victory_option_label_clamps_out_of_range(void)
{
    ASSERT_EQ_INT(strcmp(victory_option_label(TEST_BELOW_RANGE), "New game"),
                  0);
    ASSERT_EQ_INT(strcmp(victory_option_label(TEST_ABOVE_RANGE), "Main menu"),
                  0);
    ASSERT_EQ_INT(
        strcmp(victory_option_label(VICTORY_OPTION_COUNT), "Main menu"), 0);
}

int main(void)
{
    RUN_TEST(test_init_victory_state_is_hidden);
    RUN_TEST(test_is_victory_shown_follows_the_screen);
    RUN_TEST(test_open_victory_screen_shows_and_selects_new_game);
    RUN_TEST(test_open_victory_screen_builds_the_summary);
    RUN_TEST(test_open_victory_screen_twice_keeps_the_first_summary);
    RUN_TEST(test_init_victory_state_hides_a_shown_screen);
    RUN_TEST(test_apply_victory_menu_command_moves_and_stops_at_ends);
    RUN_TEST(test_apply_victory_menu_command_select_returns_selected_action);
    RUN_TEST(test_apply_victory_menu_command_shortcuts_return_their_action);
    RUN_TEST(test_apply_victory_menu_command_keeps_the_screen_shown);
    RUN_TEST(test_apply_victory_menu_command_ignored_when_hidden);
    RUN_TEST(test_apply_victory_menu_command_none_changes_nothing);
    RUN_TEST(test_build_victory_summary_formats_time_like_the_timer);
    RUN_TEST(test_build_victory_summary_clamps_the_time_like_the_timer);
    RUN_TEST(test_build_victory_summary_names_the_difficulty);
    RUN_TEST(test_build_victory_summary_counts_moves);
    RUN_TEST(test_build_victory_summary_overwrites_previous_content);
    RUN_TEST(test_build_victory_summary_lines_fit);
    RUN_TEST(test_victory_summary_constants_hold_the_longest_line);
    RUN_TEST(test_victory_option_labels);
    RUN_TEST(test_victory_option_label_clamps_out_of_range);
    return test_harness_result();
}
