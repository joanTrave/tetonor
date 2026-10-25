#ifndef TETONOR_UI_GAME_SESSION_H
#define TETONOR_UI_GAME_SESSION_H

#include <stdbool.h>
#include <time.h>

#include "keys.h"
#include "pause_menu.h"
#include "../game/game_port.h"

typedef enum
{
    GAME_SESSION_CONTINUE,
    GAME_SESSION_RESTARTED, /* a new puzzle: re-apply its palette styles */
    GAME_SESSION_LEAVE      /* back to the main menu; nothing recorded */
} GameSessionOutcome;

/* Everything one running game owns; the board points into puzzle. */
typedef struct
{
    NumberGeneration* puzzle; /* owned; released by end_game_session */
    Board board;
    GameTimer timer;
    PauseState pause;
    bool undo_notice; /* set by an empty undo, cleared by the next key */
    BoardKeyCodes board_key_codes;
    MenuKeyCodes menu_key_codes;
} GameSession;

/* Generates a puzzle through the port, starts the board, the timer (at now)
 * and an unpaused state. Returns false, owning nothing, when generation fails.
 */
bool init_game_session(const GamePort* game_port, GameSession* session,
                       const BoardKeyCodes* board_key_codes,
                       const MenuKeyCodes* menu_key_codes, time_t now);

/* Frees the puzzle through the port and sets it to NULL; calling it again
 * does nothing. */
void end_game_session(const GamePort* game_port, GameSession* session);

/* Applies one key on the current screen (game, pause menu or prompt). The
 * timer is paused while the menu or a prompt is shown. */
GameSessionOutcome handle_game_session_key(const GamePort* game_port,
                                           GameSession* session, int key,
                                           time_t now);

/* Advances the timer; true exactly once, when the game completes. */
bool update_game_session_timer(const GamePort* game_port, GameSession* session,
                               time_t now);

#endif
