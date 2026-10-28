#ifndef TETONOR_UI_GAME_SESSION_H
#define TETONOR_UI_GAME_SESSION_H

#include <stdbool.h>
#include <time.h>

#include "keys.h"
#include "pause_menu.h"
#include "theme_menu.h"
#include "victory_menu.h"
#include "../game/game_port.h"
#include "../storage/saved_game_store.h"
#include "../storage/stats_store.h"

typedef enum
{
    GAME_SESSION_CONTINUE,
    GAME_SESSION_RESTARTED,     /* a new puzzle */
    GAME_SESSION_THEME_CHANGED, /* session.theme changed: apply and save it */
    GAME_SESSION_LEAVE,         /* back to the main menu; nothing recorded */
    GAME_SESSION_SAVE           /* the caller captures and stores the game */
} GameSessionOutcome;

/* Everything one running game owns; the board points into puzzle. */
typedef struct
{
    NumberGeneration* puzzle; /* owned; released by end_game_session */
    Board board;
    GameTimer timer;
    PauseState pause;
    VictoryState victory; /* shown once the game completes */
    PaletteTheme theme;   /* active theme, recorded only */
    ThemeMenu theme_menu; /* meaningful while the theme screen is shown */
    bool undo_notice;     /* set by an empty undo, cleared by the next key */
    bool linked_to_save;  /* this game is the saved one: completing it
                             discards the save */
    BoardKeyCodes board_key_codes;
    MenuKeyCodes menu_key_codes;
} GameSession;

/* Generates a puzzle through the port, starts the board, the timer (at now)
 * and an unpaused state with no victory screen. Returns false, owning nothing,
 * when generation fails.
 */
bool init_game_session(const GamePort* game_port, GameSession* session,
                       const BoardKeyCodes* board_key_codes,
                       const MenuKeyCodes* menu_key_codes, time_t now);

/* As init_game_session, but over saved: the puzzle, entries, move count and
 * elapsed time come from the save through restore_saved_game_f (the undo
 * history starts empty), and the session is linked to the save. Returns false,
 * owning nothing, when restoring fails. */
bool init_game_session_from_save(const GamePort* game_port,
                                 GameSession* session, const SavedGame* saved,
                                 const BoardKeyCodes* board_key_codes,
                                 const MenuKeyCodes* menu_key_codes,
                                 time_t now);

/* Captures the game through capture_saved_game_f with the timer's elapsed
 * seconds and the port's difficulty. Returns false, leaving *out untouched,
 * when the game cannot be saved (always the case for a daily challenge). */
bool capture_game_session(const GamePort* game_port, const GameSession* session,
                          SavedGame* out);

/* Shows notice on the pause menu. Unless it is PAUSE_SAVE_NOTICE_FAILED, the
 * game is now stored, so the session is linked to the save. */
void note_game_session_saved(GameSession* session, PauseSaveNotice notice);

/* Records theme as the session's active theme (the default when it is not a
 * theme). Touches nothing else; the caller applies the styles. */
void set_game_session_theme(GameSession* session, int theme);

/* Frees the puzzle through the port and sets it to NULL; calling it again
 * does nothing. */
void end_game_session(const GamePort* game_port, GameSession* session);

/* Applies one key on the current screen (game, pause menu, theme screen, prompt
 * or victory screen). The timer is paused while the menu, the theme screen or a
 * prompt is shown. Returns GAME_SESSION_SAVE when the pause menu asks to save;
 * the timer stays paused. */
GameSessionOutcome handle_game_session_key(const GamePort* game_port,
                                           GameSession* session, int key,
                                           time_t now);

/* Advances the timer; true exactly once, when the game completes, which also
 * opens the victory screen with the game's summary. */
bool update_game_session_timer(const GamePort* game_port, GameSession* session,
                               time_t now);

/* Bookkeeping for a game that has just completed (update_game_session_timer
 * returned true): records it in stats_store when the port records statistics,
 * and discards the save when the session is linked to it. A daily challenge
 * touches neither store. Failures of either store are ignored, as before. */
void record_game_session_completion(const GamePort* game_port,
                                    const GameSession* session,
                                    StatsStore* stats_store,
                                    SavedGameStore* saved_game_store);

#endif
