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
static void draw_board(const GameSession* session);

int ui_run(const GamePort* game_port, StatsStore* stats_store,
           ConfigStore* config_store)
{
    GameSession session;
    BoardKeyCodes board_codes = board_key_codes();
    MenuKeyCodes menu_codes   = menu_key_codes();
    if (!init_game_session(game_port, &session, &board_codes, &menu_codes,
                           time(NULL)))
    {
        return EXIT_FAILURE;
    }
    set_game_session_theme(&session, config_store->theme);
    ReadTimeoutWatch watch;
    init_read_timeout_watch(&watch);

    timeout(TETONOR_READ_TIMEOUT_MS);
    int32_t drawn_seconds = session.timer.elapsed_seconds;
    draw_board(&session);
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
            record_stats_store_game(stats_store, game_port->difficulty,
                                    session.timer.elapsed_seconds);
        }
        if (keyed || session.timer.elapsed_seconds != drawn_seconds)
        {
            drawn_seconds = session.timer.elapsed_seconds;
            draw_board(&session);
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

static void draw_board(const GameSession* session)
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
        print_pause_screen(&session->pause, session->timer.elapsed_seconds);
    }
    else
    {
        print_layout(&session->board, session->undo_notice,
                     session->timer.elapsed_seconds);
    }
    refresh();
}

void ui_apply_theme(ConfigStore* config_store, int theme)
{
    /* A failed save is ignored: the theme still applies for the session. */
    set_config_store_theme(config_store, theme);
    apply_theme_styles(config_store->theme);
}
