#ifndef TETONOR_UI_PAUSE_MENU_H
#define TETONOR_UI_PAUSE_MENU_H

#include <stdbool.h>

#include "game_action.h"

typedef enum
{
    PAUSE_SCREEN_PLAYING, /* not paused: the board takes the keys */
    PAUSE_SCREEN_MENU,
    PAUSE_SCREEN_PROMPT /* asking before progress is lost */
} PauseScreen;

typedef enum
{
    PAUSE_OPTION_RESUME,
    PAUSE_OPTION_ABANDON,
    PAUSE_OPTION_COUNT
} PauseOption;

typedef enum
{
    PROMPT_OPTION_YES,
    PROMPT_OPTION_NO,
    PROMPT_OPTION_COUNT
} PromptOption;

typedef enum
{
    PAUSE_MENU_COMMAND_NONE, /* key ignored */
    PAUSE_MENU_COMMAND_UP,
    PAUSE_MENU_COMMAND_DOWN,
    PAUSE_MENU_COMMAND_SELECT,
    PAUSE_MENU_COMMAND_RESUME
} PauseMenuCommand;

typedef enum
{
    PROMPT_COMMAND_NONE, /* key ignored */
    PROMPT_COMMAND_UP,
    PROMPT_COMMAND_DOWN,
    PROMPT_COMMAND_SELECT,
    PROMPT_COMMAND_YES,
    PROMPT_COMMAND_NO
} PromptCommand;

typedef struct
{
    PauseScreen screen;
    PauseOption selected;  /* highlighted pause-menu option */
    PromptOption answer;   /* highlighted prompt option */
    GameAction pending;    /* what the prompt asks about; NONE otherwise */
    PauseScreen cancel_to; /* PLAYING or MENU: where No and ESC return */
} PauseState;

/* Playing, Resume highlighted, prompt answer No, nothing pending (S-1). */
void init_pause_state(PauseState* state);

/* True whenever the pause menu or a prompt is shown (S-16). */
bool is_game_paused(const PauseState* state);

/* Opens the menu with Resume highlighted; ignored unless playing (S-2, S-3). */
void open_pause_menu(PauseState* state);

/* Asks to start a new game or leave. Without risk the action is returned to
 * perform at once; with risk a prompt opens on No and NONE is returned.
 * Ignored, returning NONE, for GAME_ACTION_NONE or unless playing (S-4..S-6).
 */
GameAction request_game_action(PauseState* state, GameAction action,
                               bool progress_at_risk);

/* Applies one pause-menu command: UP and DOWN stop at the ends, RESUME and
 * SELECT on Resume return to the game, SELECT on Abandon returns
 * GAME_ACTION_MAIN_MENU, or opens a prompt when progress is at risk. Ignored
 * unless the menu is shown (S-7..S-11). */
GameAction apply_pause_menu_command(PauseState* state, PauseMenuCommand command,
                                    bool progress_at_risk);

/* Applies one prompt command: YES, or SELECT on Yes, returns the pending
 * action and resumes play; NO, or SELECT on No, goes back to where the prompt
 * was opened. Ignored unless a prompt is shown (S-12..S-15). */
GameAction apply_prompt_command(PauseState* state, PromptCommand command);

/* "Resume" or "Abandon"; out-of-range values are clamped. */
const char* pause_option_label(int option);

/* "Yes" or "No"; out-of-range values are clamped. */
const char* prompt_option_label(int option);

/* "Start a new game?" or "Abandon this game?"; "" for GAME_ACTION_NONE or an
 * out-of-range value. */
const char* prompt_question(int action);

#endif
