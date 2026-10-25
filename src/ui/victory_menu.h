#ifndef TETONOR_UI_VICTORY_MENU_H
#define TETONOR_UI_VICTORY_MENU_H

#include <stdbool.h>
#include <stdint.h>

#include "game_action.h"

#define VICTORY_SUMMARY_LINE_SIZE 32 /* longest line is 22 characters */
#define VICTORY_SUMMARY_LABEL_W 12   /* "Difficulty: ": values start here */
#define VICTORY_SUMMARY_TIME_LABEL "Time:"
#define VICTORY_SUMMARY_DIFFICULTY_LABEL "Difficulty:"
#define VICTORY_SUMMARY_MOVES_LABEL "Moves:"

typedef enum
{
    VICTORY_SUMMARY_TIME,
    VICTORY_SUMMARY_DIFFICULTY,
    VICTORY_SUMMARY_MOVES,
    VICTORY_SUMMARY_LINE_COUNT
} VictorySummaryLine;

typedef enum
{
    VICTORY_OPTION_NEW_GAME,
    VICTORY_OPTION_MAIN_MENU,
    VICTORY_OPTION_COUNT
} VictoryOption;

typedef enum
{
    VICTORY_MENU_COMMAND_NONE, /* key ignored */
    VICTORY_MENU_COMMAND_UP,
    VICTORY_MENU_COMMAND_DOWN,
    VICTORY_MENU_COMMAND_SELECT,
    VICTORY_MENU_COMMAND_NEW_GAME,
    VICTORY_MENU_COMMAND_MAIN_MENU
} VictoryMenuCommand;

/* The finished game, as text ready to draw. A fixed-size value. */
typedef struct
{
    char lines[VICTORY_SUMMARY_LINE_COUNT][VICTORY_SUMMARY_LINE_SIZE];
} VictorySummary;

typedef struct
{
    bool shown;
    VictoryOption selected; /* highlighted option */
    VictorySummary summary; /* frozen when the screen opens */
} VictoryState;

/* Hidden, New game highlighted, empty summary (V-S1). This is also the only
 * way to hide the screen again (V-S8). */
void init_victory_state(VictoryState* state);

/* True while the victory screen is shown. */
bool is_victory_shown(const VictoryState* state);

/* Shows the screen with New game highlighted and the summary of the finished
 * game; ignored while already shown, so the first summary stays (V-S2, V-S3).
 */
void open_victory_screen(VictoryState* state, int32_t elapsed_seconds,
                         int difficulty, int move_count);

/* Applies one command: UP and DOWN stop at the ends, SELECT returns the action
 * of the highlighted option, NEW_GAME and MAIN_MENU return theirs. The screen
 * stays shown. Ignored, returning GAME_ACTION_NONE, unless shown (V-S4..V-S7).
 */
GameAction apply_victory_menu_command(VictoryState* state,
                                      VictoryMenuCommand command);

/* The three summary lines: label padded to VICTORY_SUMMARY_LABEL_W, then the
 * value. The time is formatted like the in-game timer, the difficulty is
 * clamped and a negative move count shows as 0. Overwrites every line; never
 * fails. */
void build_victory_summary(int32_t elapsed_seconds, int difficulty,
                           int move_count, VictorySummary* summary);

/* "New game" or "Main menu"; out-of-range values are clamped. */
const char* victory_option_label(int option);

#endif
