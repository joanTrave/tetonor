#include <curses.h>
#include <stdlib.h>
#include <time.h>

#include "ui.h"
#include "keys.h"
#include "print_layout.h"
#include "read_timeout.h"
#include "style.h"

/* Everything one running game owns; the board points into puzzle. */
typedef struct
{
    NumberGeneration* puzzle;
    Board board;
    GameTimer timer;
    bool undo_notice; /* set by an empty undo, cleared by the next key */
    BoardKeyCodes key_codes;
} GameSession;

static BoardKeyCodes board_key_codes(void);
static void draw_board(const GameSession* session);
static bool handle_key(const GamePort* game_port, GameSession* session, int key,
                       time_t now);
static void update_timer(const GamePort* game_port, GameSession* session,
                         StatsStore* stats_store, time_t now);

int ui_run(const GamePort* game_port, StatsStore* stats_store)
{
    GameSession session;
    session.puzzle =
        game_port->generate_random_numbers_f(game_port->difficulty);
    if (session.puzzle == NULL)
    {
        return EXIT_FAILURE;
    }

    game_port->init_board_f(&session.board, session.puzzle);
    apply_palette_styles(&session.board.palette);
    session.key_codes   = board_key_codes();
    session.undo_notice = false;
    game_port->start_game_timer_f(&session.timer, time(NULL));
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
            if (!handle_key(game_port, &session, key, now))
            {
                break;
            }
        }
        update_timer(game_port, &session, stats_store, now);
        if (keyed || session.timer.elapsed_seconds != drawn_seconds)
        {
            drawn_seconds = session.timer.elapsed_seconds;
            draw_board(&session);
        }
    }

    timeout(-1);
    game_port->free_number_generation_f(session.puzzle);
    return EXIT_SUCCESS;
}

/* Applies one key. Returns false when the player left the game. */
static bool handle_key(const GamePort* game_port, GameSession* session, int key,
                       time_t now)
{
    BoardCommand command = board_command_from_key(key, &session->key_codes);
    if (command.kind == BOARD_COMMAND_EXIT)
    {
        return false;
    }
    session->undo_notice = false;
    if (command.kind == BOARD_COMMAND_UNDO)
    {
        session->undo_notice = !game_port->undo_board_move_f(&session->board);
    }
    if (command.kind == BOARD_COMMAND_INPUT)
    {
        BoardEvent event =
            game_port->apply_board_input_f(&session->board, command.input);
        if (event == BOARD_EVENT_RESTART &&
            restart_game(game_port, &session->board, &session->puzzle))
        {
            apply_palette_styles(&session->board.palette);
            game_port->start_game_timer_f(&session->timer, now);
        }
    }
    return true;
}

/* Advances the timer; a game completes once, and is recorded then. A failed
 * save is ignored: the game goes on and the next completion tries again. */
static void update_timer(const GamePort* game_port, GameSession* session,
                         StatsStore* stats_store, time_t now)
{
    if (game_port->update_game_timer_f(&session->timer, session->board.result,
                                       now))
    {
        record_stats_store_game(stats_store, game_port->difficulty,
                                session->timer.elapsed_seconds);
    }
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

static void draw_board(const GameSession* session)
{
    erase();
    print_layout(&session->board, session->undo_notice,
                 session->timer.elapsed_seconds);
    refresh();
}
