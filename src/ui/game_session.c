#include <stddef.h>

#include "game_session.h"

/* The state every session starts with, however its game began. */
static void init_session_state(GameSession* session,
                               const BoardKeyCodes* board_key_codes,
                               const MenuKeyCodes* menu_key_codes,
                               bool linked_to_save)
{
    init_pause_state(&session->pause);
    init_victory_state(&session->victory);
    session->theme = TETONOR_DEFAULT_PALETTE_THEME;
    init_theme_menu(&session->theme_menu, session->theme);
    session->undo_notice     = false;
    session->hint_shown      = false;
    session->linked_to_save  = linked_to_save;
    session->board_key_codes = *board_key_codes;
    session->menu_key_codes  = *menu_key_codes;
}

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
    init_session_state(session, board_key_codes, menu_key_codes, false);
    return true;
}

bool init_game_session_from_save(const GamePort* game_port,
                                 GameSession* session, const SavedGame* saved,
                                 const BoardKeyCodes* board_key_codes,
                                 const MenuKeyCodes* menu_key_codes, time_t now)
{
    session->puzzle = game_port->restore_saved_game_f(saved, &session->board,
                                                      &session->timer, now);
    if (session->puzzle == NULL)
    {
        return false;
    }
    init_session_state(session, board_key_codes, menu_key_codes, true);
    return true;
}

bool capture_game_session(const GamePort* game_port, const GameSession* session,
                          SavedGame* out)
{
    return game_port->capture_saved_game_f(&session->board,
                                           session->timer.elapsed_seconds,
                                           game_port->difficulty, out);
}

void note_game_session_saved(GameSession* session, PauseSaveNotice notice)
{
    set_pause_save_notice(&session->pause, notice);
    if (notice != PAUSE_SAVE_NOTICE_FAILED)
    {
        session->linked_to_save = true;
    }
}

void set_game_session_theme(GameSession* session, int theme)
{
    session->theme = is_palette_theme(theme) ? (PaletteTheme)theme
                                             : TETONOR_DEFAULT_PALETTE_THEME;
}

bool game_session_counts_for_statistics(const GamePort* game_port,
                                        const GameSession* session)
{
    return game_port->board_counts_for_statistics_f(&session->board);
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
    init_victory_state(&session->victory);
    session->hint_shown     = false;
    session->linked_to_save = false;
    return true;
}

static bool progress_at_risk(const GamePort* game_port,
                             const GameSession* session)
{
    return game_port->board_progress_would_be_lost_f(&session->board);
}

/* Opens the theme screen with the active theme marked and highlighted. */
static void open_session_theme_screen(GameSession* session)
{
    init_theme_menu(&session->theme_menu, session->theme);
    open_theme_screen(&session->pause);
}

/* Undoes the latest move unless the board is locked. A move that changed the
 * entries ends the hint on screen; an empty undo shows the undo notice. */
static void undo_move(const GamePort* game_port, GameSession* session)
{
    if (game_port->is_board_locked_f(&session->board))
    {
        return;
    }
    session->undo_notice = !game_port->undo_board_move_f(&session->board);
    if (!session->undo_notice)
    {
        session->hint_shown = false;
    }
}

/* Applies one board input. A hint on screen stays only while the entries stay
 * the same, i.e. while the move count does. */
static BoardEvent apply_input(const GamePort* game_port, GameSession* session,
                              BoardInput input)
{
    int moves_before = game_port->board_move_count_f(&session->board);
    BoardEvent event = game_port->apply_board_input_f(&session->board, input);
    if (game_port->board_move_count_f(&session->board) != moves_before)
    {
        session->hint_shown = false;
    }
    return event;
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
        case BOARD_COMMAND_THEME:
            open_session_theme_screen(session);
            break;
        case BOARD_COMMAND_NEW_GAME:
            return request_game_action(&session->pause, GAME_ACTION_NEW_GAME,
                                       progress_at_risk(game_port, session));
        case BOARD_COMMAND_MAIN_MENU:
            return request_game_action(&session->pause, GAME_ACTION_MAIN_MENU,
                                       progress_at_risk(game_port, session));
        case BOARD_COMMAND_UNDO:
            undo_move(game_port, session);
            break;
        case BOARD_COMMAND_HINT:
            session->hint = game_port->request_board_hint_f(&session->board);
            session->hint_shown = true;
            break;
        case BOARD_COMMAND_INPUT:
            if (apply_input(game_port, session, command.input) ==
                    BOARD_EVENT_RESTART &&
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
        {
            PauseMenuCommand command =
                pause_menu_command_from_key(key, &session->menu_key_codes);
            if (command == PAUSE_MENU_COMMAND_THEME)
            {
                init_theme_menu(&session->theme_menu, session->theme);
            }
            return apply_pause_menu_command(
                &session->pause, command, progress_at_risk(game_port, session));
        }
        case PAUSE_SCREEN_PROMPT:
            return apply_prompt_command(
                &session->pause,
                prompt_command_from_key(key, &session->menu_key_codes));
        case PAUSE_SCREEN_THEME:
        case PAUSE_SCREEN_PLAYING:
            break;
    }
    return GAME_ACTION_NONE;
}

/* Applies a key on the theme screen. Choosing another theme records it and
 * reports it; the board, the history and the result are never touched. */
static GameSessionOutcome handle_theme_key(GameSession* session, int key)
{
    apply_theme_menu_command(
        &session->theme_menu,
        theme_menu_command_from_key(key, &session->menu_key_codes));
    switch (session->theme_menu.status)
    {
        case THEME_MENU_OPEN:
            break;
        case THEME_MENU_CHOSEN:
            close_theme_screen(&session->pause);
            if (session->theme_menu.selected != session->theme)
            {
                session->theme = session->theme_menu.selected;
                return GAME_SESSION_THEME_CHANGED;
            }
            break;
        case THEME_MENU_CANCELLED:
            close_theme_screen(&session->pause);
            break;
    }
    return GAME_SESSION_CONTINUE;
}

static GameAction handle_victory_key(GameSession* session, int key)
{
    return apply_victory_menu_command(
        &session->victory,
        victory_menu_command_from_key(key, &session->menu_key_codes));
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
        case GAME_ACTION_SAVE:
            return GAME_SESSION_SAVE;
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
    GameAction action          = GAME_ACTION_NONE;
    session->undo_notice       = false;
    if (is_victory_shown(&session->victory))
    {
        action = handle_victory_key(session, key);
    }
    else if (is_theme_screen_shown(&session->pause))
    {
        outcome = handle_theme_key(session, key);
    }
    else if (was_paused)
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
    if (!game_port->update_game_timer_f(&session->timer, session->board.result,
                                        now))
    {
        return false;
    }
    open_victory_screen(&session->victory, session->timer.elapsed_seconds,
                        game_port->difficulty,
                        game_port->board_move_count_f(&session->board));
    return true;
}
