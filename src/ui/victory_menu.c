#include <stdio.h>
#include <string.h>

#include "victory_menu.h"
#include "difficulty_menu.h"
#include "../game/game_port.h"

static const char* const victory_labels[VICTORY_OPTION_COUNT] = {
    [VICTORY_OPTION_NEW_GAME]  = "New game",
    [VICTORY_OPTION_MAIN_MENU] = "Main menu",
};

static int clamp_option(int option, int count)
{
    if (option < 0)
    {
        return 0;
    }
    if (option > count - 1)
    {
        return count - 1;
    }
    return option;
}

static void format_summary_line(char* out, const char* label, const char* value)
{
    snprintf(out, VICTORY_SUMMARY_LINE_SIZE, "%-*s%s", VICTORY_SUMMARY_LABEL_W,
             label, value);
}

static GameAction selected_action(const VictoryState* state)
{
    switch (state->selected)
    {
        case VICTORY_OPTION_NEW_GAME:
            return GAME_ACTION_NEW_GAME;
        case VICTORY_OPTION_MAIN_MENU:
            return GAME_ACTION_MAIN_MENU;
        case VICTORY_OPTION_COUNT:
            break;
    }
    return GAME_ACTION_NONE;
}

static GameAction apply_shown_command(VictoryState* state,
                                      VictoryMenuCommand command)
{
    switch (command)
    {
        case VICTORY_MENU_COMMAND_UP:
            if (state->selected > VICTORY_OPTION_NEW_GAME)
            {
                state->selected--;
            }
            break;
        case VICTORY_MENU_COMMAND_DOWN:
            if (state->selected < VICTORY_OPTION_MAIN_MENU)
            {
                state->selected++;
            }
            break;
        case VICTORY_MENU_COMMAND_SELECT:
            return selected_action(state);
        case VICTORY_MENU_COMMAND_NEW_GAME:
            return GAME_ACTION_NEW_GAME;
        case VICTORY_MENU_COMMAND_MAIN_MENU:
            return GAME_ACTION_MAIN_MENU;
        case VICTORY_MENU_COMMAND_NONE:
            break;
    }
    return GAME_ACTION_NONE;
}

void init_victory_state(VictoryState* state)
{
    state->shown    = false;
    state->selected = VICTORY_OPTION_NEW_GAME;
    memset(&state->summary, 0, sizeof state->summary);
}

bool is_victory_shown(const VictoryState* state)
{
    return state->shown;
}

void open_victory_screen(VictoryState* state, int32_t elapsed_seconds,
                         int difficulty, int move_count)
{
    if (state->shown)
    {
        return;
    }
    state->shown    = true;
    state->selected = VICTORY_OPTION_NEW_GAME;
    build_victory_summary(elapsed_seconds, difficulty, move_count,
                          &state->summary);
}

GameAction apply_victory_menu_command(VictoryState* state,
                                      VictoryMenuCommand command)
{
    if (!state->shown)
    {
        return GAME_ACTION_NONE;
    }
    return apply_shown_command(state, command);
}

void build_victory_summary(int32_t elapsed_seconds, int difficulty,
                           int move_count, VictorySummary* summary)
{
    char time_text[TETONOR_ELAPSED_TEXT_SIZE];
    char moves_text[VICTORY_SUMMARY_LINE_SIZE];
    memset(summary, 0, sizeof *summary);
    format_elapsed_time(elapsed_seconds, time_text, sizeof time_text);
    snprintf(moves_text, sizeof moves_text, "%d",
             move_count > 0 ? move_count : 0);
    format_summary_line(summary->lines[VICTORY_SUMMARY_TIME],
                        VICTORY_SUMMARY_TIME_LABEL, time_text);
    format_summary_line(summary->lines[VICTORY_SUMMARY_DIFFICULTY],
                        VICTORY_SUMMARY_DIFFICULTY_LABEL,
                        difficulty_menu_label(difficulty));
    format_summary_line(summary->lines[VICTORY_SUMMARY_MOVES],
                        VICTORY_SUMMARY_MOVES_LABEL, moves_text);
}

const char* victory_option_label(int option)
{
    return victory_labels[clamp_option(option, VICTORY_OPTION_COUNT)];
}
