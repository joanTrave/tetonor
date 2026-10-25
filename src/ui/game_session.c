#include <stddef.h>

#include "game_session.h"

bool init_game_session(const GamePort* game_port, GameSession* session,
                       const BoardKeyCodes* board_key_codes,
                       const MenuKeyCodes* menu_key_codes, time_t now)
{
    session->puzzle =
        game_port->generate_random_numbers_f(game_port->difficulty);
    if (session->puzzle == NULL)
    {
        return false;
    }
    game_port->init_board_f(&session->board, session->puzzle);
    game_port->start_game_timer_f(&session->timer, now);
    init_pause_state(&session->pause);
    session->undo_notice     = false;
    session->board_key_codes = *board_key_codes;
    session->menu_key_codes  = *menu_key_codes;
    return true;
}

void end_game_session(const GamePort* game_port, GameSession* session)
{
    if (session->puzzle == NULL)
    {
        return;
    }
    game_port->free_number_generation_f(session->puzzle);
    session->puzzle = NULL;
}

/* A new puzzle through the port, with the timer back at zero. On failure the
 * game goes on untouched. */
static bool restart_new_game(const GamePort* game_port, GameSession* session,
                             time_t now)
{
    if (!restart_game(game_port, &session->board, &session->puzzle))
    {
        return false;
    }
    game_port->start_game_timer_f(&session->timer, now);
    return true;
}

static bool progress_at_risk(const GamePort* game_port,
                             const GameSession* session)
{
    return game_port->board_progress_would_be_lost_f(&session->board);
}

/* Applies a board key while playing. Sets *outcome for ESC and for the RESTART
 * control; returns the action the pause logic wants performed now. */
static GameAction handle_board_key(const GamePort* game_port,
                                   GameSession* session, int key,
                                   GameSessionOutcome* outcome, time_t now)
{
    BoardCommand command =
        board_command_from_key(key, &session->board_key_codes);
    switch (command.kind)
    {
        case BOARD_COMMAND_EXIT:
            *outcome = GAME_SESSION_LEAVE;
            break;
        case BOARD_COMMAND_PAUSE:
            open_pause_menu(&session->pause);
            break;
        case BOARD_COMMAND_NEW_GAME:
            return request_game_action(&session->pause, GAME_ACTION_NEW_GAME,
                                       progress_at_risk(game_port, session));
        case BOARD_COMMAND_MAIN_MENU:
            return request_game_action(&session->pause, GAME_ACTION_MAIN_MENU,
                                       progress_at_risk(game_port, session));
        case BOARD_COMMAND_UNDO:
            if (!game_port->is_board_locked_f(&session->board))
            {
                session->undo_notice =
                    !game_port->undo_board_move_f(&session->board);
            }
            break;
        case BOARD_COMMAND_INPUT:
            if (game_port->apply_board_input_f(
                    &session->board, command.input) == BOARD_EVENT_RESTART &&
                restart_new_game(game_port, session, now))
            {
                *outcome = GAME_SESSION_RESTARTED;
            }
            break;
        case BOARD_COMMAND_NONE:
            break;
    }
    return GAME_ACTION_NONE;
}

static GameAction handle_pause_key(const GamePort* game_port,
                                   GameSession* session, int key)
{
    switch (session->pause.screen)
    {
        case PAUSE_SCREEN_MENU:
            return apply_pause_menu_command(
                &session->pause,
                pause_menu_command_from_key(key, &session->menu_key_codes),
                progress_at_risk(game_port, session));
        case PAUSE_SCREEN_PROMPT:
            return apply_prompt_command(
                &session->pause,
                prompt_command_from_key(key, &session->menu_key_codes));
        case PAUSE_SCREEN_PLAYING:
            break;
    }
    return GAME_ACTION_NONE;
}

/* The timer stands still exactly while the pause menu or a prompt is shown. */
static void sync_pause_timer(const GamePort* game_port, GameSession* session,
                             bool was_paused, time_t now)
{
    bool paused = is_game_paused(&session->pause);
    if (!was_paused && paused)
    {
        game_port->pause_game_timer_f(&session->timer, now);
    }
    else if (was_paused && !paused)
    {
        game_port->resume_game_timer_f(&session->timer, now);
    }
}

static GameSessionOutcome
perform_game_action(const GamePort* game_port, GameSession* session,
                    GameAction action, GameSessionOutcome outcome, time_t now)
{
    switch (action)
    {
        case GAME_ACTION_NEW_GAME:
            return restart_new_game(game_port, session, now)
                       ? GAME_SESSION_RESTARTED
                       : GAME_SESSION_CONTINUE;
        case GAME_ACTION_MAIN_MENU:
            return GAME_SESSION_LEAVE;
        case GAME_ACTION_NONE:
            break;
    }
    return outcome;
}

GameSessionOutcome handle_game_session_key(const GamePort* game_port,
                                           GameSession* session, int key,
                                           time_t now)
{
    bool was_paused            = is_game_paused(&session->pause);
    GameSessionOutcome outcome = GAME_SESSION_CONTINUE;
    GameAction action;
    session->undo_notice = false;
    if (was_paused)
    {
        action = handle_pause_key(game_port, session, key);
    }
    else
    {
        action = handle_board_key(game_port, session, key, &outcome, now);
    }
    sync_pause_timer(game_port, session, was_paused, now);
    return perform_game_action(game_port, session, action, outcome, now);
}

bool update_game_session_timer(const GamePort* game_port, GameSession* session,
                               time_t now)
{
    return game_port->update_game_timer_f(&session->timer,
                                          session->board.result, now);
}
