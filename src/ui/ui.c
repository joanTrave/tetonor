#include <curses.h>
#include <stdlib.h>
#include <time.h>

#include "ui.h"
#include "game_session.h"
#include "keys.h"
#include "print_layout.h"
#include "read_timeout.h"
#include "style.h"

static BoardKeyCodes board_key_codes(void);
static MenuKeyCodes menu_key_codes(void);
static void draw_board(const GamePort* game_port, const GameSession* session);
static void save_session(const GamePort* game_port, GameSession* session,
                         SavedGameStore* saved_game_store);

int ui_run(const GamePort* game_port, StatsStore* stats_store,
           ConfigStore* config_store, SavedGameStore* saved_game_store,
           const SavedGame* resume)
{
    GameSession session;
    BoardKeyCodes board_codes = board_key_codes();
    MenuKeyCodes menu_codes   = menu_key_codes();
    time_t started            = time(NULL);
    bool ready =
        resume == NULL
            ? init_game_session(game_port, &session, &board_codes, &menu_codes,
                                started)
            : init_game_session_from_save(game_port, &session, resume,
                                          &board_codes, &menu_codes, started);
    if (!ready)
    {
        return EXIT_FAILURE;
    }
    set_game_session_theme(&session, config_store->theme);
    ReadTimeoutWatch watch;
    init_read_timeout_watch(&watch);

    timeout(TETONOR_READ_TIMEOUT_MS);
    int32_t drawn_seconds = session.timer.elapsed_seconds;
    draw_board(game_port, &session);
    for (;;)
    {
        int key    = getch();
        time_t now = time(NULL);
        bool keyed = key != ERR;
        if (!keyed && note_read_error(&watch, now))
        {
            break;
        }
        if (keyed)
        {
            note_key_read(&watch);
            GameSessionOutcome outcome =
                handle_game_session_key(game_port, &session, key, now);
            if (outcome == GAME_SESSION_LEAVE)
            {
                break;
            }
            if (outcome == GAME_SESSION_SAVE)
            {
                save_session(game_port, &session, saved_game_store);
            }
            if (outcome == GAME_SESSION_THEME_CHANGED)
            {
                ui_apply_theme(config_store, session.theme);
            }
        }
        /* A game completes once: it is recorded then and the victory screen
         * opens. A failed save is ignored: the screen still shows and the next
         * completion tries again. */
        if (update_game_session_timer(game_port, &session, now))
        {
            record_game_session_completion(game_port, &session, stats_store,
                                           saved_game_store);
        }
        if (keyed || session.timer.elapsed_seconds != drawn_seconds)
        {
            drawn_seconds = session.timer.elapsed_seconds;
            draw_board(game_port, &session);
        }
    }

    timeout(-1);
    end_game_session(game_port, &session);
    return EXIT_SUCCESS;
}

/* The only feature-macro guards: PDCurses-only keypad codes. */
static BoardKeyCodes board_key_codes(void)
{
    BoardKeyCodes key_codes;
    key_codes.enter     = KEY_ENTER;
    key_codes.left      = KEY_LEFT;
    key_codes.right     = KEY_RIGHT;
    key_codes.backspace = KEY_BACKSPACE;
    key_codes.up        = KEY_UP;
    key_codes.down      = KEY_DOWN;
#ifdef PADENTER
    key_codes.pad_enter = PADENTER;
#else
    key_codes.pad_enter = TETONOR_KEY_UNAVAILABLE;
#endif
#ifdef PAD0
    key_codes.pad_zero = PAD0;
#else
    key_codes.pad_zero = TETONOR_KEY_UNAVAILABLE;
#endif
#ifdef PADPLUS
    key_codes.pad_plus = PADPLUS;
#else
    key_codes.pad_plus = TETONOR_KEY_UNAVAILABLE;
#endif
#ifdef PADSTAR
    key_codes.pad_star = PADSTAR;
#else
    key_codes.pad_star = TETONOR_KEY_UNAVAILABLE;
#endif
    return key_codes;
}

static MenuKeyCodes menu_key_codes(void)
{
    MenuKeyCodes key_codes;
    key_codes.enter = KEY_ENTER;
    key_codes.up    = KEY_UP;
    key_codes.down  = KEY_DOWN;
    return key_codes;
}

static void draw_board(const GamePort* game_port, const GameSession* session)
{
    erase();
    if (is_victory_shown(&session->victory))
    {
        print_victory_screen(&session->victory);
    }
    else if (is_theme_screen_shown(&session->pause))
    {
        print_theme_screen(&session->theme_menu);
    }
    else if (is_game_paused(&session->pause))
    {
        print_pause_screen(&session->pause, session->timer.elapsed_seconds,
                           game_port_allows_saving(game_port));
    }
    else
    {
        print_layout(&session->board, session->undo_notice,
                     session->hint_shown ? &session->hint : NULL,
                     !game_session_counts_for_statistics(game_port, session),
                     session->timer.elapsed_seconds);
    }
    if (game_port->mode == GAME_MODE_DAILY_CHALLENGE)
    {
        print_daily_challenge_badge(game_port->daily_date);
    }
    refresh();
}

void ui_apply_theme(ConfigStore* config_store, int theme)
{
    /* A failed save is ignored: the theme still applies for the session. */
    set_config_store_theme(config_store, theme);
    apply_theme_styles(config_store->theme);
}

static PauseSaveNotice save_notice(SavedGameSaveResult result)
{
    switch (result)
    {
        case SAVED_GAME_SAVE_STORED:
            return PAUSE_SAVE_NOTICE_SAVED;
        case SAVED_GAME_SAVE_SESSION_ONLY:
            return PAUSE_SAVE_NOTICE_SESSION_ONLY;
        case SAVED_GAME_SAVE_REJECTED:
            break;
    }
    return PAUSE_SAVE_NOTICE_FAILED;
}

/* Stores the paused game and reports on the pause menu how it went. A game
 * that cannot be captured or is rejected is not saved, and play goes on. */
static void save_session(const GamePort* game_port, GameSession* session,
                         SavedGameStore* saved_game_store)
{
    SavedGame saved;
    PauseSaveNotice notice = PAUSE_SAVE_NOTICE_FAILED;
    if (capture_game_session(game_port, session, &saved))
    {
        notice = save_notice(save_saved_game_store(saved_game_store, &saved));
    }
    note_game_session_saved(session, notice);
}
