#include "pause_menu.h"

static const char* const pause_labels[PAUSE_OPTION_COUNT] = {
    [PAUSE_OPTION_RESUME]  = "Resume",
    [PAUSE_OPTION_ABANDON] = "Abandon",
};

static const char* const prompt_labels[PROMPT_OPTION_COUNT] = {
    [PROMPT_OPTION_YES] = "Yes",
    [PROMPT_OPTION_NO]  = "No",
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

static void open_prompt(PauseState* state, GameAction action,
                        PauseScreen cancel_to)
{
    state->screen    = PAUSE_SCREEN_PROMPT;
    state->pending   = action;
    state->answer    = PROMPT_OPTION_NO;
    state->cancel_to = cancel_to;
}

static GameAction select_pause_option(PauseState* state, bool progress_at_risk)
{
    if (state->selected == PAUSE_OPTION_RESUME)
    {
        state->screen = PAUSE_SCREEN_PLAYING;
        return GAME_ACTION_NONE;
    }
    if (progress_at_risk)
    {
        open_prompt(state, GAME_ACTION_MAIN_MENU, PAUSE_SCREEN_MENU);
        return GAME_ACTION_NONE;
    }
    state->screen = PAUSE_SCREEN_PLAYING;
    return GAME_ACTION_MAIN_MENU;
}

static GameAction apply_open_menu_command(PauseState* state,
                                          PauseMenuCommand command,
                                          bool progress_at_risk)
{
    switch (command)
    {
        case PAUSE_MENU_COMMAND_UP:
            if (state->selected > PAUSE_OPTION_RESUME)
            {
                state->selected--;
            }
            break;
        case PAUSE_MENU_COMMAND_DOWN:
            if (state->selected < PAUSE_OPTION_ABANDON)
            {
                state->selected++;
            }
            break;
        case PAUSE_MENU_COMMAND_RESUME:
            state->screen = PAUSE_SCREEN_PLAYING;
            break;
        case PAUSE_MENU_COMMAND_THEME:
            open_theme_screen(state);
            break;
        case PAUSE_MENU_COMMAND_SELECT:
            return select_pause_option(state, progress_at_risk);
        case PAUSE_MENU_COMMAND_NONE:
            break;
    }
    return GAME_ACTION_NONE;
}

static GameAction answer_prompt(PauseState* state, bool yes)
{
    GameAction action = state->pending;
    state->pending    = GAME_ACTION_NONE;
    if (yes)
    {
        state->screen = PAUSE_SCREEN_PLAYING;
        return action;
    }
    state->screen = state->cancel_to;
    return GAME_ACTION_NONE;
}

static GameAction apply_open_prompt_command(PauseState* state,
                                            PromptCommand command)
{
    switch (command)
    {
        case PROMPT_COMMAND_UP:
            state->answer = PROMPT_OPTION_YES;
            break;
        case PROMPT_COMMAND_DOWN:
            state->answer = PROMPT_OPTION_NO;
            break;
        case PROMPT_COMMAND_YES:
            return answer_prompt(state, true);
        case PROMPT_COMMAND_NO:
            return answer_prompt(state, false);
        case PROMPT_COMMAND_SELECT:
            return answer_prompt(state, state->answer == PROMPT_OPTION_YES);
        case PROMPT_COMMAND_NONE:
            break;
    }
    return GAME_ACTION_NONE;
}

void init_pause_state(PauseState* state)
{
    state->screen    = PAUSE_SCREEN_PLAYING;
    state->selected  = PAUSE_OPTION_RESUME;
    state->answer    = PROMPT_OPTION_NO;
    state->pending   = GAME_ACTION_NONE;
    state->cancel_to = PAUSE_SCREEN_PLAYING;
}

bool is_game_paused(const PauseState* state)
{
    return state->screen != PAUSE_SCREEN_PLAYING;
}

void open_pause_menu(PauseState* state)
{
    if (state->screen != PAUSE_SCREEN_PLAYING)
    {
        return;
    }
    state->screen   = PAUSE_SCREEN_MENU;
    state->selected = PAUSE_OPTION_RESUME;
}

void open_theme_screen(PauseState* state)
{
    if (state->screen != PAUSE_SCREEN_PLAYING &&
        state->screen != PAUSE_SCREEN_MENU)
    {
        return;
    }
    state->cancel_to = state->screen;
    state->screen    = PAUSE_SCREEN_THEME;
}

void close_theme_screen(PauseState* state)
{
    if (state->screen != PAUSE_SCREEN_THEME)
    {
        return;
    }
    state->screen    = state->cancel_to;
    state->cancel_to = PAUSE_SCREEN_PLAYING;
}

bool is_theme_screen_shown(const PauseState* state)
{
    return state->screen == PAUSE_SCREEN_THEME;
}

GameAction request_game_action(PauseState* state, GameAction action,
                               bool progress_at_risk)
{
    if (state->screen != PAUSE_SCREEN_PLAYING || action == GAME_ACTION_NONE)
    {
        return GAME_ACTION_NONE;
    }
    if (!progress_at_risk)
    {
        return action;
    }
    open_prompt(state, action, PAUSE_SCREEN_PLAYING);
    return GAME_ACTION_NONE;
}

GameAction apply_pause_menu_command(PauseState* state, PauseMenuCommand command,
                                    bool progress_at_risk)
{
    if (state->screen != PAUSE_SCREEN_MENU)
    {
        return GAME_ACTION_NONE;
    }
    return apply_open_menu_command(state, command, progress_at_risk);
}

GameAction apply_prompt_command(PauseState* state, PromptCommand command)
{
    if (state->screen != PAUSE_SCREEN_PROMPT)
    {
        return GAME_ACTION_NONE;
    }
    return apply_open_prompt_command(state, command);
}

const char* pause_option_label(int option)
{
    return pause_labels[clamp_option(option, PAUSE_OPTION_COUNT)];
}

const char* prompt_option_label(int option)
{
    return prompt_labels[clamp_option(option, PROMPT_OPTION_COUNT)];
}

const char* prompt_question(int action)
{
    switch (action)
    {
        case GAME_ACTION_NEW_GAME:
            return "Start a new game?";
        case GAME_ACTION_MAIN_MENU:
            return "Abandon this game?";
        default:
            return "";
    }
}
