#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../test_harness.h"
#include "../game/fixture_puzzle.h"
#include "../game/fixture_saved_game.h"
#include "../../src/ui/game_session.h"

#define TEST_ENTER_KEY 0x157
#define TEST_PAD_ENTER_KEY 0x1cb
#define TEST_LEFT_KEY 0x104
#define TEST_RIGHT_KEY 0x105
#define TEST_BACKSPACE_KEY 0x107
#define TEST_PAD_ZERO_KEY 0x1fa
#define TEST_UP_KEY 0x103
#define TEST_DOWN_KEY 0x102
#define TEST_PAD_PLUS_KEY 0x1d1
#define TEST_PAD_STAR_KEY 0x1cf
#define TEST_RESIZE_KEY 0x19a
#define TEST_ERR_KEY (-1)
#define TEST_ESCAPE 27
#define TEST_DELETE_KEY 127

#define TEST_START_TIME ((time_t)100)
#define TEST_HIDDEN_COUNT 4
#define TEST_FIRST_SLOT 2
#define TEST_SECOND_SLOT 5
#define TEST_THIRD_SLOT 6
#define TEST_LAST_SLOT 15
#define TEST_TYPED_DIGIT '7'
#define TEST_OTHER_DIGIT '3'
#define TEST_FAKE_PUZZLE_DIFFICULTY_UNSET (-1)

static const int fixture_hidden[TEST_HIDDEN_COUNT] = {
    TEST_FIRST_SLOT, TEST_SECOND_SLOT, TEST_THIRD_SLOT, TEST_LAST_SLOT};

static const BoardKeyCodes board_codes = {.enter     = TEST_ENTER_KEY,
                                          .pad_enter = TEST_PAD_ENTER_KEY,
                                          .left      = TEST_LEFT_KEY,
                                          .right     = TEST_RIGHT_KEY,
                                          .backspace = TEST_BACKSPACE_KEY,
                                          .pad_zero  = TEST_PAD_ZERO_KEY,
                                          .up        = TEST_UP_KEY,
                                          .down      = TEST_DOWN_KEY,
                                          .pad_plus  = TEST_PAD_PLUS_KEY,
                                          .pad_star  = TEST_PAD_STAR_KEY};

static const MenuKeyCodes menu_codes   = {
    .enter = TEST_ENTER_KEY, .up = TEST_UP_KEY, .down = TEST_DOWN_KEY};

static int fake_generate_calls;
static int fake_generate_difficulty;
static bool fake_generate_fails;
static int fake_alloc_count;
static int fake_free_calls;

static NumberGeneration* fake_generate(int difficulty)
{
    fake_generate_calls++;
    fake_generate_difficulty = difficulty;
    if (fake_generate_fails)
    {
        return NULL;
    }
    NumberGeneration* puzzle = malloc(sizeof *puzzle);
    if (puzzle != NULL)
    {
        fake_alloc_count++;
        build_fixture_puzzle(puzzle, fixture_hidden, TEST_HIDDEN_COUNT);
    }
    return puzzle;
}

static void fake_free(NumberGeneration* puzzle)
{
    fake_free_calls++;
    free(puzzle);
}

static GamePort* create_fake_port(void)
{
    fake_generate_calls      = 0;
    fake_generate_difficulty = TEST_FAKE_PUZZLE_DIFFICULTY_UNSET;
    fake_generate_fails      = false;
    fake_alloc_count         = 0;
    fake_free_calls          = 0;
    GamePort* game_port      = create_game_port();
    if (game_port != NULL)
    {
        game_port->generate_random_numbers_f = &fake_generate;
        game_port->free_number_generation_f  = &fake_free;
    }
    return game_port;
}

static bool start_session(const GamePort* game_port, GameSession* session,
                          time_t now)
{
    return init_game_session(game_port, session, &board_codes, &menu_codes,
                             now);
}

/* Runs body over a started session, then releases everything and checks that
 * every generated puzzle was freed exactly once. */
static void run_session_test(void (*body)(const GamePort*, GameSession*))
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    GameSession session;
    if (!start_session(game_port, &session, TEST_START_TIME))
    {
        destroy_game_port(game_port);
        ASSERT_TRUE(false);
    }
    body(game_port, &session);
    end_game_session(game_port, &session);
    destroy_game_port(game_port);
    ASSERT_EQ_INT(fake_free_calls, fake_alloc_count);
}

static GameSessionOutcome press(const GamePort* game_port, GameSession* session,
                                int key, time_t now)
{
    return handle_game_session_key(game_port, session, key, now);
}

static void press_all(const GamePort* game_port, GameSession* session,
                      const int* keys, size_t count, time_t now)
{
    for (size_t i = 0; i < count; i++)
    {
        handle_game_session_key(game_port, session, keys[i], now);
    }
}

/* One undoable edit: a digit in the slot under the cursor. */
static void make_edit(const GamePort* game_port, GameSession* session)
{
    press(game_port, session, TEST_TYPED_DIGIT, TEST_START_TIME);
}

/* White-box: the solution groups are set directly; the slots are typed through
 * keys and CONFIRM presses the submit control. Needs a fresh board. */
static void solve_session(const GamePort* game_port, GameSession* session,
                          time_t now)
{
    memcpy(session->board.groups, fixture_groups, sizeof session->board.groups);
    const int keys[] = {'2',
                        TEST_RIGHT_KEY,
                        '5',
                        TEST_RIGHT_KEY,
                        '6',
                        TEST_RIGHT_KEY,
                        '2',
                        '9',
                        TEST_RIGHT_KEY,
                        TEST_ENTER_KEY};
    press_all(game_port, session, keys, sizeof keys / sizeof keys[0], now);
}

/* Slots typed, groups filled, focus on the submit control, not yet judged. */
static void fill_without_confirming(const GamePort* game_port,
                                    GameSession* session, time_t now)
{
    memcpy(session->board.groups, fixture_groups, sizeof session->board.groups);
    const int keys[] = {'2',           TEST_RIGHT_KEY, '5', TEST_RIGHT_KEY,
                        '6',           TEST_RIGHT_KEY, '2', '9',
                        TEST_RIGHT_KEY};
    press_all(game_port, session, keys, sizeof keys / sizeof keys[0], now);
}

static bool groups_match(const OperandGroup* first, const OperandGroup* second)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (first[i].smaller != second[i].smaller ||
            first[i].operation != second[i].operation ||
            first[i].greater != second[i].greater)
        {
            return false;
        }
    }
    return true;
}

/* Field by field: padding bytes of a Board are unspecified. Everything an edit
 * can change, the move history included; cursor and focus are left out. */
static bool entries_match(const Board* first, const Board* second)
{
    if (first->result != second->result ||
        first->history.count != second->history.count ||
        first->history.newest != second->history.newest)
    {
        return false;
    }
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (first->slots[i].editable != second->slots[i].editable ||
            (first->slots[i].editable &&
             first->slots[i].value != second->slots[i].value) ||
            first->slot_usages[i] != second->slot_usages[i])
        {
            return false;
        }
    }
    return groups_match(first->groups, second->groups);
}

static bool boards_match(const Board* first, const Board* second)
{
    return first->puzzle == second->puzzle && first->cursor == second->cursor &&
           first->group_cursor == second->group_cursor &&
           first->group_field == second->group_field &&
           first->focus == second->focus && entries_match(first, second);
}

static bool timers_match(const GameTimer* first, const GameTimer* second)
{
    return first->elapsed_seconds == second->elapsed_seconds &&
           first->last_tick == second->last_tick &&
           first->running == second->running && first->paused == second->paused;
}

static bool pause_states_match(const PauseState* first,
                               const PauseState* second)
{
    return first->screen == second->screen &&
           first->selected == second->selected &&
           first->answer == second->answer &&
           first->pending == second->pending &&
           first->cancel_to == second->cancel_to;
}

static bool session_is_fresh(const GameSession* session, time_t now)
{
    Board expected;
    init_board(&expected, session->puzzle);
    return session->board.puzzle == session->puzzle &&
           boards_match(&session->board, &expected) &&
           session->board.history.count == 0 &&
           session->timer.elapsed_seconds == 0 && session->timer.running &&
           !session->timer.paused && session->timer.last_tick == now &&
           session->pause.screen == PAUSE_SCREEN_PLAYING &&
           !session->undo_notice;
}

static void test_init_game_session_starts_playing(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    GameSession session;
    bool started       = start_session(game_port, &session, TEST_START_TIME);
    int calls          = fake_generate_calls;
    int level          = fake_generate_difficulty;
    bool fresh         = started && session_is_fresh(&session, TEST_START_TIME);
    bool keys_kept     = started &&
                         session.board_key_codes.enter == TEST_ENTER_KEY &&
                         session.board_key_codes.up == TEST_UP_KEY &&
                         session.menu_key_codes.enter == TEST_ENTER_KEY &&
                         session.menu_key_codes.down == TEST_DOWN_KEY;
    BoardResult result = session.board.result;
    if (started)
    {
        end_game_session(game_port, &session);
    }
    int freed      = fake_free_calls;
    int port_level = game_port->difficulty;
    destroy_game_port(game_port);
    ASSERT_TRUE(started);
    ASSERT_EQ_INT(calls, 1);
    ASSERT_EQ_INT(level, port_level);
    ASSERT_TRUE(fresh);
    ASSERT_TRUE(keys_kept);
    ASSERT_EQ_INT(result, BOARD_RESULT_PENDING);
    ASSERT_EQ_INT(freed, 1);
}

static void test_init_game_session_generation_failure_owns_nothing(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    fake_generate_fails = true;
    GameSession session;
    bool started = start_session(game_port, &session, TEST_START_TIME);
    int calls    = fake_generate_calls;
    int freed    = fake_free_calls;
    destroy_game_port(game_port);
    ASSERT_FALSE(started);
    ASSERT_EQ_INT(calls, 1);
    ASSERT_EQ_INT(freed, 0);
}

static void test_end_game_session_frees_once(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    GameSession session;
    if (!start_session(game_port, &session, TEST_START_TIME))
    {
        destroy_game_port(game_port);
        ASSERT_TRUE(false);
    }
    end_game_session(game_port, &session);
    int freed_once = fake_free_calls;
    bool released  = session.puzzle == NULL;
    end_game_session(game_port, &session);
    int freed_twice = fake_free_calls;
    destroy_game_port(game_port);
    ASSERT_EQ_INT(freed_once, 1);
    ASSERT_TRUE(released);
    ASSERT_EQ_INT(freed_twice, 1);
}

static void check_escape_leaves_at_once_without_edits(const GamePort* game_port,
                                                      GameSession* session)
{
    ASSERT_EQ_INT(press(game_port, session, TEST_ESCAPE, TEST_START_TIME),
                  GAME_SESSION_LEAVE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
}

static void check_escape_leaves_at_once_with_edits(const GamePort* game_port,
                                                   GameSession* session)
{
    make_edit(game_port, session);
    ASSERT_TRUE(session->board.history.count > 0);
    ASSERT_EQ_INT(press(game_port, session, TEST_ESCAPE, TEST_START_TIME),
                  GAME_SESSION_LEAVE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_EQ_INT(fake_generate_calls, 1);
}

static void test_handle_game_session_key_escape_leaves_at_once(void)
{
    run_session_test(check_escape_leaves_at_once_without_edits);
    run_session_test(check_escape_leaves_at_once_with_edits);
}

static void check_p_pauses(const GamePort* game_port, GameSession* session)
{
    ASSERT_EQ_INT(press(game_port, session, 'p', TEST_START_TIME + 5),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_MENU);
    ASSERT_EQ_INT(session->pause.selected, PAUSE_OPTION_RESUME);
    ASSERT_TRUE(session->timer.paused);
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 5);
    ASSERT_EQ_INT(press(game_port, session, TEST_ESCAPE, TEST_START_TIME + 5),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_FALSE(session->timer.paused);
    ASSERT_EQ_INT(press(game_port, session, 'P', TEST_START_TIME + 5),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_MENU);
    ASSERT_TRUE(session->timer.paused);
}

static void test_handle_game_session_key_p_pauses(void)
{
    run_session_test(check_p_pauses);
}

static const int keys_a_board_would_take[] = {'0',
                                              '1',
                                              '5',
                                              '9',
                                              TEST_LEFT_KEY,
                                              TEST_RIGHT_KEY,
                                              TEST_UP_KEY,
                                              TEST_DOWN_KEY,
                                              '+',
                                              TEST_PAD_PLUS_KEY,
                                              'x',
                                              'X',
                                              '*',
                                              TEST_PAD_STAR_KEY,
                                              '\b',
                                              TEST_DELETE_KEY,
                                              TEST_BACKSPACE_KEY,
                                              TEST_PAD_ZERO_KEY,
                                              'u',
                                              'U'};

static void
assert_board_keys_change_nothing_while_paused(const GamePort* game_port,
                                              GameSession* session)
{
    Board board_before = session->board;
    GameTimer timer    = session->timer;
    ASSERT_TRUE(timer.paused);
    for (size_t i = 0;
         i < sizeof keys_a_board_would_take / sizeof keys_a_board_would_take[0];
         i++)
    {
        GameSessionOutcome outcome =
            press(game_port, session, keys_a_board_would_take[i],
                  TEST_START_TIME + 1000);
        ASSERT_EQ_INT(outcome, GAME_SESSION_CONTINUE);
        ASSERT_TRUE(boards_match(&session->board, &board_before));
        ASSERT_TRUE(timers_match(&session->timer, &timer));
        ASSERT_FALSE(session->undo_notice);
        ASSERT_TRUE(session->pause.screen != PAUSE_SCREEN_PLAYING);
    }
}

static void
check_board_keys_ignored_in_the_pause_menu(const GamePort* game_port,
                                           GameSession* session)
{
    make_edit(game_port, session);
    press(game_port, session, 'p', TEST_START_TIME + 2);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_MENU);
    assert_board_keys_change_nothing_while_paused(game_port, session);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_MENU);
}

static void check_board_keys_ignored_in_the_prompt(const GamePort* game_port,
                                                   GameSession* session)
{
    make_edit(game_port, session);
    press(game_port, session, 'N', TEST_START_TIME + 2);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PROMPT);
    assert_board_keys_change_nothing_while_paused(game_port, session);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PROMPT);
    ASSERT_EQ_INT(fake_generate_calls, 1);
}

static void test_handle_game_session_key_board_keys_ignored_while_paused(void)
{
    run_session_test(check_board_keys_ignored_in_the_pause_menu);
    run_session_test(check_board_keys_ignored_in_the_prompt);
}

static void check_enter_while_paused_never_confirms(const GamePort* game_port,
                                                    GameSession* session)
{
    fill_without_confirming(game_port, session, TEST_START_TIME);
    ASSERT_EQ_INT(session->board.focus, BOARD_FOCUS_SUBMIT);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_PENDING);
    Board before = session->board;

    press(game_port, session, 'P', TEST_START_TIME);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_MENU);
    ASSERT_EQ_INT(press(game_port, session, TEST_ENTER_KEY, TEST_START_TIME),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_PENDING);
    ASSERT_TRUE(boards_match(&session->board, &before));

    press(game_port, session, 'N', TEST_START_TIME);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PROMPT);
    ASSERT_EQ_INT(press(game_port, session, TEST_ENTER_KEY, TEST_START_TIME),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_PENDING);
    ASSERT_TRUE(boards_match(&session->board, &before));
}

static void test_handle_game_session_key_enter_while_paused_never_confirms(void)
{
    run_session_test(check_enter_while_paused_never_confirms);
}

static void check_resume_restores_the_board(const GamePort* game_port,
                                            GameSession* session)
{
    press(game_port, session, TEST_TYPED_DIGIT, TEST_START_TIME);
    press(game_port, session, TEST_RIGHT_KEY, TEST_START_TIME);
    press(game_port, session, TEST_OTHER_DIGIT, TEST_START_TIME);
    press(game_port, session, TEST_UP_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(session->board.focus, BOARD_FOCUS_GROUP);
    Board before            = session->board;
    const int resume_keys[] = {TEST_ESCAPE, 'p', 'P', TEST_ENTER_KEY, ' '};
    for (size_t i = 0; i < sizeof resume_keys / sizeof resume_keys[0]; i++)
    {
        press(game_port, session, 'p', TEST_START_TIME);
        ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_MENU);
        ASSERT_EQ_INT(
            press(game_port, session, resume_keys[i], TEST_START_TIME),
            GAME_SESSION_CONTINUE);
        ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
        ASSERT_TRUE(boards_match(&session->board, &before));
        ASSERT_EQ_INT(session->board.history.count, before.history.count);
    }
    ASSERT_EQ_INT(session->board.slots[TEST_FIRST_SLOT].value,
                  TEST_TYPED_DIGIT - '0');
    ASSERT_EQ_INT(session->board.slots[TEST_SECOND_SLOT].value,
                  TEST_OTHER_DIGIT - '0');
    press(game_port, session, 'u', TEST_START_TIME);
    ASSERT_FALSE(session->undo_notice);
    ASSERT_EQ_INT(session->board.slots[TEST_SECOND_SLOT].value,
                  TETONOR_EMPTY_ENTRY);
    ASSERT_EQ_INT(session->board.slots[TEST_FIRST_SLOT].value,
                  TEST_TYPED_DIGIT - '0');
}

static void test_handle_game_session_key_resume_restores_board(void)
{
    run_session_test(check_resume_restores_the_board);
}

static void check_pause_freezes_the_timer(const GamePort* game_port,
                                          GameSession* session)
{
    ASSERT_FALSE(
        update_game_session_timer(game_port, session, TEST_START_TIME + 5));
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 5);
    press(game_port, session, 'P', TEST_START_TIME + 5);
    ASSERT_FALSE(
        update_game_session_timer(game_port, session, TEST_START_TIME + 6));
    ASSERT_FALSE(update_game_session_timer(game_port, session,
                                           TEST_START_TIME + 5 + 3600));
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 5);
    press(game_port, session, TEST_ESCAPE, 4000);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_FALSE(update_game_session_timer(game_port, session, 4002));
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 7);
}

static void
check_pause_counts_the_time_before_the_key(const GamePort* game_port,
                                           GameSession* session)
{
    press(game_port, session, 'p', TEST_START_TIME + 7);
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 7);
    ASSERT_FALSE(update_game_session_timer(game_port, session, 300));
    press(game_port, session, 'p', 500);
    ASSERT_FALSE(update_game_session_timer(game_port, session, 501));
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 8);
}

static void test_handle_game_session_key_pause_freezes_timer(void)
{
    run_session_test(check_pause_freezes_the_timer);
    run_session_test(check_pause_counts_the_time_before_the_key);
}

static void check_prompt_freezes_the_timer(const GamePort* game_port,
                                           GameSession* session)
{
    press(game_port, session, TEST_TYPED_DIGIT, TEST_START_TIME + 1);
    ASSERT_FALSE(
        update_game_session_timer(game_port, session, TEST_START_TIME + 3));
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 3);
    press(game_port, session, 'N', TEST_START_TIME + 4);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PROMPT);
    ASSERT_TRUE(session->timer.paused);
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 4);
    ASSERT_FALSE(update_game_session_timer(game_port, session, 1000));
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 4);
    press(game_port, session, 'n', 2000);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_FALSE(session->timer.paused);
    ASSERT_FALSE(update_game_session_timer(game_port, session, 2002));
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 6);
}

static void test_handle_game_session_key_prompt_freezes_timer(void)
{
    run_session_test(check_prompt_freezes_the_timer);
}

static void check_menu_navigation(const GamePort* game_port,
                                  GameSession* session)
{
    press(game_port, session, 'p', TEST_START_TIME);
    ASSERT_EQ_INT(session->pause.selected, PAUSE_OPTION_RESUME);
    press(game_port, session, TEST_UP_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(session->pause.selected, PAUSE_OPTION_RESUME);
    press(game_port, session, TEST_DOWN_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(session->pause.selected, PAUSE_OPTION_ABANDON);
    press(game_port, session, TEST_DOWN_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(session->pause.selected, PAUSE_OPTION_ABANDON);
    press(game_port, session, TEST_UP_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(session->pause.selected, PAUSE_OPTION_RESUME);
    press(game_port, session, TEST_DOWN_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(press(game_port, session, TEST_ESCAPE, TEST_START_TIME),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    press(game_port, session, 'p', TEST_START_TIME);
    ASSERT_EQ_INT(session->pause.selected, PAUSE_OPTION_RESUME);
}

static void test_handle_game_session_key_menu_navigation(void)
{
    run_session_test(check_menu_navigation);
}

static void check_new_game_without_progress(const GamePort* game_port,
                                            GameSession* session)
{
    const char new_game_keys[] = {'N', 'n'};
    update_game_session_timer(game_port, session, TEST_START_TIME + 50);
    press(game_port, session, TEST_RIGHT_KEY, TEST_START_TIME + 50);
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 50);
    for (size_t i = 0; i < sizeof new_game_keys; i++)
    {
        time_t now                  = 200 + (time_t)i;
        const NumberGeneration* old = session->puzzle;
        int calls_before            = fake_generate_calls;
        int frees_before            = fake_free_calls;
        ASSERT_EQ_INT(press(game_port, session, new_game_keys[i], now),
                      GAME_SESSION_RESTARTED);
        ASSERT_EQ_INT(fake_generate_calls, calls_before + 1);
        ASSERT_EQ_INT(fake_free_calls, frees_before + 1);
        ASSERT_TRUE(session->puzzle != old);
        ASSERT_TRUE(session_is_fresh(session, now));
    }
}

static void test_handle_game_session_key_new_game_without_progress(void)
{
    run_session_test(check_new_game_without_progress);
}

static void check_new_game_with_progress_asks(const GamePort* game_port,
                                              GameSession* session)
{
    make_edit(game_port, session);
    Board before        = session->board;
    const int cancels[] = {'n', 'N', TEST_ESCAPE, TEST_ENTER_KEY, ' '};
    for (size_t i = 0; i < sizeof cancels / sizeof cancels[0]; i++)
    {
        ASSERT_EQ_INT(press(game_port, session, 'N', TEST_START_TIME + 1),
                      GAME_SESSION_CONTINUE);
        ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PROMPT);
        ASSERT_EQ_INT(session->pause.answer, PROMPT_OPTION_NO);
        ASSERT_EQ_INT(fake_generate_calls, 1);
        ASSERT_EQ_INT(
            press(game_port, session, cancels[i], TEST_START_TIME + 1),
            GAME_SESSION_CONTINUE);
        ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
        ASSERT_TRUE(boards_match(&session->board, &before));
        ASSERT_EQ_INT(fake_generate_calls, 1);
        ASSERT_EQ_INT(fake_free_calls, 0);
    }
    const int yes_keys[] = {'y', 'Y'};
    for (size_t i = 0; i < sizeof yes_keys / sizeof yes_keys[0]; i++)
    {
        make_edit(game_port, session);
        press(game_port, session, 'N', 300);
        ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PROMPT);
        ASSERT_EQ_INT(press(game_port, session, yes_keys[i], 301),
                      GAME_SESSION_RESTARTED);
        ASSERT_TRUE(session_is_fresh(session, 301));
    }

    make_edit(game_port, session);
    press(game_port, session, 'N', 400);
    press(game_port, session, TEST_UP_KEY, 400);
    ASSERT_EQ_INT(session->pause.answer, PROMPT_OPTION_YES);
    ASSERT_EQ_INT(press(game_port, session, TEST_ENTER_KEY, 401),
                  GAME_SESSION_RESTARTED);
    ASSERT_TRUE(session_is_fresh(session, 401));
    ASSERT_EQ_INT(fake_generate_calls, 4);
    ASSERT_EQ_INT(fake_free_calls, 3);
}

static void test_handle_game_session_key_new_game_with_progress_asks(void)
{
    run_session_test(check_new_game_with_progress_asks);
}

static void check_main_menu_without_progress(const GamePort* game_port,
                                             GameSession* session)
{
    ASSERT_EQ_INT(press(game_port, session, 'M', TEST_START_TIME),
                  GAME_SESSION_LEAVE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_EQ_INT(press(game_port, session, 'm', TEST_START_TIME),
                  GAME_SESSION_LEAVE);
    ASSERT_EQ_INT(fake_generate_calls, 1);
}

static void test_handle_game_session_key_main_menu_without_progress_leaves(void)
{
    run_session_test(check_main_menu_without_progress);
}

static void check_main_menu_with_progress_asks(const GamePort* game_port,
                                               GameSession* session)
{
    make_edit(game_port, session);
    Board before        = session->board;
    const int cancels[] = {'n', 'N', TEST_ESCAPE, TEST_ENTER_KEY};
    for (size_t i = 0; i < sizeof cancels / sizeof cancels[0]; i++)
    {
        ASSERT_EQ_INT(press(game_port, session, 'M', TEST_START_TIME),
                      GAME_SESSION_CONTINUE);
        ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PROMPT);
        ASSERT_EQ_INT(press(game_port, session, cancels[i], TEST_START_TIME),
                      GAME_SESSION_CONTINUE);
        ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
        ASSERT_TRUE(boards_match(&session->board, &before));
    }
    press(game_port, session, 'm', TEST_START_TIME);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PROMPT);
    ASSERT_EQ_INT(press(game_port, session, 'y', TEST_START_TIME),
                  GAME_SESSION_LEAVE);
    ASSERT_EQ_INT(fake_generate_calls, 1);
}

static void test_handle_game_session_key_main_menu_with_progress_asks(void)
{
    run_session_test(check_main_menu_with_progress_asks);
}

static void check_abandon_without_progress(const GamePort* game_port,
                                           GameSession* session)
{
    ASSERT_EQ_INT(press(game_port, session, 'P', TEST_START_TIME),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(press(game_port, session, TEST_DOWN_KEY, TEST_START_TIME),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(press(game_port, session, TEST_ENTER_KEY, TEST_START_TIME),
                  GAME_SESSION_LEAVE);
    ASSERT_EQ_INT(fake_generate_calls, 1);
}

static void test_handle_game_session_key_abandon_without_progress_leaves(void)
{
    run_session_test(check_abandon_without_progress);
}

static void check_abandon_with_progress_asks(const GamePort* game_port,
                                             GameSession* session)
{
    make_edit(game_port, session);
    Board before = session->board;
    press(game_port, session, 'P', TEST_START_TIME);
    press(game_port, session, TEST_DOWN_KEY, TEST_START_TIME);
    const int cancels[] = {'n', TEST_ESCAPE, TEST_ENTER_KEY};
    for (size_t i = 0; i < sizeof cancels / sizeof cancels[0]; i++)
    {
        ASSERT_EQ_INT(
            press(game_port, session, TEST_ENTER_KEY, TEST_START_TIME),
            GAME_SESSION_CONTINUE);
        ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PROMPT);
        ASSERT_EQ_INT(press(game_port, session, cancels[i], TEST_START_TIME),
                      GAME_SESSION_CONTINUE);
        ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_MENU);
        ASSERT_EQ_INT(session->pause.selected, PAUSE_OPTION_ABANDON);
        ASSERT_TRUE(session->timer.paused);
        ASSERT_TRUE(boards_match(&session->board, &before));
    }
    press(game_port, session, TEST_ENTER_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PROMPT);
    ASSERT_EQ_INT(press(game_port, session, 'y', TEST_START_TIME),
                  GAME_SESSION_LEAVE);
    ASSERT_EQ_INT(fake_generate_calls, 1);
}

static void test_handle_game_session_key_abandon_with_progress_asks(void)
{
    run_session_test(check_abandon_with_progress_asks);
}

static const int keys_that_would_edit_a_board[] = {TEST_ENTER_KEY,
                                                   TEST_PAD_ENTER_KEY,
                                                   'u',
                                                   'U',
                                                   '0',
                                                   '1',
                                                   '5',
                                                   '9',
                                                   '+',
                                                   'x',
                                                   'X',
                                                   '*',
                                                   '\b',
                                                   TEST_DELETE_KEY,
                                                   TEST_BACKSPACE_KEY,
                                                   TEST_PAD_PLUS_KEY,
                                                   TEST_PAD_STAR_KEY,
                                                   TEST_PAD_ZERO_KEY};

static void assert_edit_keys_change_nothing(const GamePort* game_port,
                                            GameSession* session)
{
    Board before = session->board;
    for (size_t i = 0; i < sizeof keys_that_would_edit_a_board /
                               sizeof keys_that_would_edit_a_board[0];
         i++)
    {
        GameSessionOutcome outcome =
            press(game_port, session, keys_that_would_edit_a_board[i],
                  TEST_START_TIME);
        ASSERT_EQ_INT(outcome, GAME_SESSION_CONTINUE);
        ASSERT_TRUE(boards_match(&session->board, &before));
        ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
        ASSERT_FALSE(session->undo_notice);
        ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    }
}

static void check_solved_board_ignores_edits(const GamePort* game_port,
                                             GameSession* session)
{
    solve_session(game_port, session, TEST_START_TIME);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
    ASSERT_EQ_INT(session->board.focus, BOARD_FOCUS_SUBMIT);
    ASSERT_TRUE(session->board.history.count > 0);
    assert_edit_keys_change_nothing(game_port, session);

    press(game_port, session, TEST_UP_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(session->board.focus, BOARD_FOCUS_SLOT);
    assert_edit_keys_change_nothing(game_port, session);

    press(game_port, session, TEST_UP_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(session->board.focus, BOARD_FOCUS_GROUP);
    assert_edit_keys_change_nothing(game_port, session);

    press(game_port, session, TEST_RIGHT_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(session->board.group_field, GROUP_FIELD_OPERATION);
    assert_edit_keys_change_nothing(game_port, session);

    ASSERT_EQ_INT(session->board.slots[TEST_LAST_SLOT].value, 29);
    ASSERT_TRUE(groups_match(session->board.groups, fixture_groups));
}

static void test_handle_game_session_key_solved_board_ignores_edits(void)
{
    run_session_test(check_solved_board_ignores_edits);
}

static void check_solved_board_arrows_still_move(const GamePort* game_port,
                                                 GameSession* session)
{
    solve_session(game_port, session, TEST_START_TIME);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
    Board before = session->board;
    ASSERT_EQ_INT(press(game_port, session, TEST_UP_KEY, TEST_START_TIME),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->board.focus, BOARD_FOCUS_SLOT);
    ASSERT_EQ_INT(session->board.cursor, TEST_LAST_SLOT);
    press(game_port, session, TEST_LEFT_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(session->board.cursor, TEST_THIRD_SLOT);
    press(game_port, session, TEST_RIGHT_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(session->board.cursor, TEST_LAST_SLOT);
    press(game_port, session, TEST_DOWN_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(session->board.focus, BOARD_FOCUS_SUBMIT);
    press(game_port, session, TEST_DOWN_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(session->board.focus, BOARD_FOCUS_RESTART);
    ASSERT_TRUE(entries_match(&session->board, &before));
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
}

static void test_handle_game_session_key_solved_board_arrows_still_move(void)
{
    run_session_test(check_solved_board_arrows_still_move);
}

static void check_solved_board_can_pause(const GamePort* game_port,
                                         GameSession* session)
{
    solve_session(game_port, session, TEST_START_TIME);
    Board before = session->board;
    ASSERT_EQ_INT(press(game_port, session, 'P', TEST_START_TIME),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_MENU);
    ASSERT_EQ_INT(press(game_port, session, TEST_ESCAPE, TEST_START_TIME),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_TRUE(boards_match(&session->board, &before));
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
}

static void test_handle_game_session_key_solved_board_can_pause(void)
{
    run_session_test(check_solved_board_can_pause);
}

static void
check_solved_board_new_game_starts_without_asking(const GamePort* game_port,
                                                  GameSession* session)
{
    solve_session(game_port, session, TEST_START_TIME);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(session->board.history.count > 0);
    ASSERT_EQ_INT(press(game_port, session, 'N', 500), GAME_SESSION_RESTARTED);
    ASSERT_EQ_INT(fake_generate_calls, 2);
    ASSERT_TRUE(session_is_fresh(session, 500));
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_PENDING);
    make_edit(game_port, session);
    ASSERT_EQ_INT(session->board.slots[TEST_FIRST_SLOT].value,
                  TEST_TYPED_DIGIT - '0');
}

static void
test_handle_game_session_key_solved_board_new_game_without_asking(void)
{
    run_session_test(check_solved_board_new_game_starts_without_asking);
}

static void
check_solved_board_main_menu_leaves_without_asking(const GamePort* game_port,
                                                   GameSession* session)
{
    solve_session(game_port, session, TEST_START_TIME);
    ASSERT_EQ_INT(press(game_port, session, 'M', TEST_START_TIME),
                  GAME_SESSION_LEAVE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
}

static void
check_solved_board_abandon_leaves_without_asking(const GamePort* game_port,
                                                 GameSession* session)
{
    solve_session(game_port, session, TEST_START_TIME);
    press(game_port, session, 'P', TEST_START_TIME);
    press(game_port, session, TEST_DOWN_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(press(game_port, session, TEST_ENTER_KEY, TEST_START_TIME),
                  GAME_SESSION_LEAVE);
}

static void check_solved_board_escape_leaves(const GamePort* game_port,
                                             GameSession* session)
{
    solve_session(game_port, session, TEST_START_TIME);
    ASSERT_EQ_INT(press(game_port, session, TEST_ESCAPE, TEST_START_TIME),
                  GAME_SESSION_LEAVE);
}

static void
check_solved_board_restart_button_restarts(const GamePort* game_port,
                                           GameSession* session)
{
    solve_session(game_port, session, TEST_START_TIME);
    press(game_port, session, TEST_DOWN_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(session->board.focus, BOARD_FOCUS_RESTART);
    ASSERT_EQ_INT(press(game_port, session, TEST_ENTER_KEY, 600),
                  GAME_SESSION_RESTARTED);
    ASSERT_TRUE(session_is_fresh(session, 600));
}

static void test_handle_game_session_key_solved_board_keeps_shortcuts(void)
{
    run_session_test(check_solved_board_main_menu_leaves_without_asking);
    run_session_test(check_solved_board_abandon_leaves_without_asking);
    run_session_test(check_solved_board_escape_leaves);
    run_session_test(check_solved_board_restart_button_restarts);
}

static void
check_completed_game_is_not_recorded_again(const GamePort* game_port,
                                           GameSession* session)
{
    solve_session(game_port, session, TEST_START_TIME + 8);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(
        update_game_session_timer(game_port, session, TEST_START_TIME + 10));
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 10);
    ASSERT_FALSE(session->timer.running);
    ASSERT_FALSE(
        update_game_session_timer(game_port, session, TEST_START_TIME + 20));

    ASSERT_EQ_INT(press(game_port, session, 'P', TEST_START_TIME + 30),
                  GAME_SESSION_CONTINUE);
    ASSERT_TRUE(is_victory_shown(&session->victory));
    ASSERT_FALSE(session->timer.paused);
    ASSERT_FALSE(update_game_session_timer(game_port, session, 500));
    ASSERT_FALSE(update_game_session_timer(game_port, session, 700));
    ASSERT_FALSE(update_game_session_timer(game_port, session, 800));
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 10);
    ASSERT_FALSE(session->timer.running);
}

static void
test_handle_game_session_key_completed_game_is_not_recorded_again(void)
{
    run_session_test(check_completed_game_is_not_recorded_again);
}

static void check_update_completes_once_when_solved(const GamePort* game_port,
                                                    GameSession* session)
{
    ASSERT_FALSE(
        update_game_session_timer(game_port, session, TEST_START_TIME + 3));
    solve_session(game_port, session, TEST_START_TIME + 3);
    ASSERT_TRUE(
        update_game_session_timer(game_port, session, TEST_START_TIME + 4));
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 4);
    ASSERT_FALSE(
        update_game_session_timer(game_port, session, TEST_START_TIME + 5));
}

static void test_update_game_session_timer_completes_once_when_solved(void)
{
    run_session_test(check_update_completes_once_when_solved);
}

static void check_restart_button_never_asks(const GamePort* game_port,
                                            GameSession* session)
{
    make_edit(game_port, session);
    ASSERT_TRUE(session->board.history.count > 0);
    press(game_port, session, TEST_DOWN_KEY, TEST_START_TIME);
    press(game_port, session, TEST_DOWN_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(session->board.focus, BOARD_FOCUS_RESTART);
    ASSERT_EQ_INT(press(game_port, session, TEST_ENTER_KEY, 300),
                  GAME_SESSION_RESTARTED);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_EQ_INT(fake_generate_calls, 2);
    ASSERT_TRUE(session_is_fresh(session, 300));
}

static void test_handle_game_session_key_restart_button_never_asks(void)
{
    run_session_test(check_restart_button_never_asks);
}

static void check_new_game_failure_keeps_the_game(const GamePort* game_port,
                                                  GameSession* session)
{
    make_edit(game_port, session);
    update_game_session_timer(game_port, session, TEST_START_TIME + 5);
    Board before                   = session->board;
    const NumberGeneration* puzzle = session->puzzle;
    fake_generate_fails            = true;
    press(game_port, session, 'N', TEST_START_TIME + 6);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PROMPT);
    ASSERT_EQ_INT(press(game_port, session, 'y', TEST_START_TIME + 7),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(fake_generate_calls, 2);
    ASSERT_EQ_INT(fake_free_calls, 0);
    ASSERT_TRUE(session->puzzle == puzzle);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_TRUE(boards_match(&session->board, &before));
    ASSERT_TRUE(session->timer.running);
    ASSERT_FALSE(session->timer.paused);
    ASSERT_FALSE(
        update_game_session_timer(game_port, session, TEST_START_TIME + 9));
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 8);
}

static void test_handle_game_session_key_new_game_failure_keeps_game(void)
{
    run_session_test(check_new_game_failure_keeps_the_game);
}

static const int keys_nobody_binds[] = {'q', 0, TEST_ERR_KEY, TEST_RESIZE_KEY,
                                        TETONOR_KEY_UNAVAILABLE};

static void assert_unknown_keys_change_nothing(const GamePort* game_port,
                                               GameSession* session,
                                               PauseScreen screen)
{
    Board board_before = session->board;
    GameTimer timer    = session->timer;
    PauseState pause   = session->pause;
    ASSERT_EQ_INT(pause.screen, screen);
    for (size_t i = 0;
         i < sizeof keys_nobody_binds / sizeof keys_nobody_binds[0]; i++)
    {
        GameSessionOutcome outcome =
            press(game_port, session, keys_nobody_binds[i], TEST_START_TIME);
        ASSERT_EQ_INT(outcome, GAME_SESSION_CONTINUE);
        ASSERT_TRUE(boards_match(&session->board, &board_before));
        ASSERT_TRUE(timers_match(&session->timer, &timer));
        ASSERT_TRUE(pause_states_match(&session->pause, &pause));
        ASSERT_FALSE(session->undo_notice);
    }
}

static void check_unknown_keys_change_nothing(const GamePort* game_port,
                                              GameSession* session)
{
    make_edit(game_port, session);
    assert_unknown_keys_change_nothing(game_port, session,
                                       PAUSE_SCREEN_PLAYING);
    press(game_port, session, 'p', TEST_START_TIME);
    assert_unknown_keys_change_nothing(game_port, session, PAUSE_SCREEN_MENU);
    press(game_port, session, TEST_ESCAPE, TEST_START_TIME);
    press(game_port, session, 'n', TEST_START_TIME);
    assert_unknown_keys_change_nothing(game_port, session, PAUSE_SCREEN_PROMPT);
    ASSERT_EQ_INT(fake_generate_calls, 1);
}

static void test_handle_game_session_key_unknown_keys_change_nothing(void)
{
    run_session_test(check_unknown_keys_change_nothing);
}

static void check_game_shortcuts_ignored_while_paused(const GamePort* game_port,
                                                      GameSession* session)
{
    make_edit(game_port, session);
    const int shortcuts[] = {'n', 'N', 'm', 'M', 'u', 'U'};
    press(game_port, session, 'p', TEST_START_TIME);
    Board board_before = session->board;
    GameTimer timer    = session->timer;
    PauseState pause   = session->pause;
    for (size_t i = 0; i < sizeof shortcuts / sizeof shortcuts[0]; i++)
    {
        ASSERT_EQ_INT(press(game_port, session, shortcuts[i], TEST_START_TIME),
                      GAME_SESSION_CONTINUE);
        ASSERT_TRUE(boards_match(&session->board, &board_before));
        ASSERT_TRUE(timers_match(&session->timer, &timer));
        ASSERT_TRUE(pause_states_match(&session->pause, &pause));
        ASSERT_FALSE(session->undo_notice);
    }

    press(game_port, session, TEST_ESCAPE, TEST_START_TIME);
    press(game_port, session, 'n', TEST_START_TIME);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PROMPT);
    const int prompt_ignored[] = {'p', 'P', 'm', 'M', 'u', 'U'};
    pause                      = session->pause;
    for (size_t i = 0; i < sizeof prompt_ignored / sizeof prompt_ignored[0];
         i++)
    {
        ASSERT_EQ_INT(
            press(game_port, session, prompt_ignored[i], TEST_START_TIME),
            GAME_SESSION_CONTINUE);
        ASSERT_TRUE(boards_match(&session->board, &board_before));
        ASSERT_TRUE(pause_states_match(&session->pause, &pause));
    }
    ASSERT_EQ_INT(fake_generate_calls, 1);
}

static void
test_handle_game_session_key_game_shortcuts_ignored_while_paused(void)
{
    run_session_test(check_game_shortcuts_ignored_while_paused);
}

static void
check_undo_notice_is_cleared_by_the_next_key(const GamePort* game_port,
                                             GameSession* session)
{
    ASSERT_EQ_INT(press(game_port, session, 'U', TEST_START_TIME),
                  GAME_SESSION_CONTINUE);
    ASSERT_TRUE(session->undo_notice);
    press(game_port, session, TEST_RIGHT_KEY, TEST_START_TIME);
    ASSERT_FALSE(session->undo_notice);
    press(game_port, session, 'u', TEST_START_TIME);
    ASSERT_TRUE(session->undo_notice);
    press(game_port, session, 'q', TEST_START_TIME);
    ASSERT_FALSE(session->undo_notice);
    press(game_port, session, 'u', TEST_START_TIME);
    ASSERT_TRUE(session->undo_notice);
    press(game_port, session, 'p', TEST_START_TIME);
    ASSERT_FALSE(session->undo_notice);
}

static void test_handle_game_session_key_undo_notice_cleared_by_next_key(void)
{
    run_session_test(check_undo_notice_is_cleared_by_the_next_key);
}

static void check_undo_reverts_the_latest_edit(const GamePort* game_port,
                                               GameSession* session)
{
    make_edit(game_port, session);
    ASSERT_EQ_INT(session->board.slots[TEST_FIRST_SLOT].value,
                  TEST_TYPED_DIGIT - '0');
    press(game_port, session, 'u', TEST_START_TIME);
    ASSERT_FALSE(session->undo_notice);
    ASSERT_EQ_INT(session->board.slots[TEST_FIRST_SLOT].value,
                  TETONOR_EMPTY_ENTRY);
}

static void test_handle_game_session_key_undo_reverts_the_latest_edit(void)
{
    run_session_test(check_undo_reverts_the_latest_edit);
}

#define TEST_SOLVED_AT (TEST_START_TIME + 8)
#define TEST_COMPLETED_AT (TEST_START_TIME + 10)
#define TEST_SOLVE_MOVES 5 /* the five digits typed by solve_session */
#define TEST_RESTART_AT 500
#define TEST_NO_EDITS_YET 0

static bool victory_states_match(const VictoryState* first,
                                 const VictoryState* second)
{
    if (first->shown != second->shown || first->selected != second->selected)
    {
        return false;
    }
    for (int line = 0; line < VICTORY_SUMMARY_LINE_COUNT; line++)
    {
        if (strcmp(first->summary.lines[line], second->summary.lines[line]) !=
            0)
        {
            return false;
        }
    }
    return true;
}

static int moves_of(const GamePort* game_port, const GameSession* session)
{
    return game_port->board_move_count_f(&session->board);
}

/* Solved, judged and completed by the timer update: the victory screen is
 * open. */
static void complete_session(const GamePort* game_port, GameSession* session)
{
    solve_session(game_port, session, TEST_SOLVED_AT);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(
        update_game_session_timer(game_port, session, TEST_COMPLETED_AT));
    ASSERT_TRUE(is_victory_shown(&session->victory));
}

static bool summary_line_is(const GameSession* session, VictorySummaryLine line,
                            const char* expected)
{
    return strcmp(session->victory.summary.lines[line], expected) == 0;
}

/* A fresh game has no victory screen and no moves, whatever led to it. */
static bool session_has_no_victory_or_moves(const GamePort* game_port,
                                            const GameSession* session)
{
    return !is_victory_shown(&session->victory) &&
           session->victory.selected == VICTORY_OPTION_NEW_GAME &&
           moves_of(game_port, session) == 0 && session->board.move_count == 0;
}

static void test_init_game_session_hides_victory(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    GameSession session;
    bool started = start_session(game_port, &session, TEST_START_TIME);
    bool clean =
        started && session_has_no_victory_or_moves(game_port, &session);
    if (started)
    {
        end_game_session(game_port, &session);
    }
    destroy_game_port(game_port);
    ASSERT_TRUE(started);
    ASSERT_TRUE(clean);
}

static void check_no_victory_while_pending_or_wrong(const GamePort* game_port,
                                                    GameSession* session)
{
    ASSERT_FALSE(
        update_game_session_timer(game_port, session, TEST_START_TIME + 1));
    ASSERT_FALSE(is_victory_shown(&session->victory));

    fill_without_confirming(game_port, session, TEST_START_TIME + 2);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_PENDING);
    ASSERT_FALSE(
        update_game_session_timer(game_port, session, TEST_START_TIME + 3));
    ASSERT_FALSE(is_victory_shown(&session->victory));

    const int make_wrong[] = {TEST_UP_KEY, TEST_DELETE_KEY, '8', TEST_RIGHT_KEY,
                              TEST_ENTER_KEY};
    press_all(game_port, session, make_wrong,
              sizeof make_wrong / sizeof make_wrong[0], TEST_START_TIME + 4);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_WRONG);
    ASSERT_FALSE(
        update_game_session_timer(game_port, session, TEST_START_TIME + 5));
    ASSERT_FALSE(is_victory_shown(&session->victory));
    ASSERT_TRUE(session->timer.running);

    const int make_correct[] = {TEST_UP_KEY, TEST_DELETE_KEY, '9',
                                TEST_RIGHT_KEY, TEST_ENTER_KEY};
    press_all(game_port, session, make_correct,
              sizeof make_correct / sizeof make_correct[0],
              TEST_START_TIME + 6);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
    ASSERT_FALSE(is_victory_shown(&session->victory));
    ASSERT_TRUE(
        update_game_session_timer(game_port, session, TEST_START_TIME + 7));
    ASSERT_TRUE(is_victory_shown(&session->victory));
}

static void
test_update_game_session_timer_pending_or_wrong_shows_no_victory(void)
{
    run_session_test(check_no_victory_while_pending_or_wrong);
}

static void check_completion_opens_victory(const GamePort* game_port,
                                           GameSession* session)
{
    solve_session(game_port, session, TEST_SOLVED_AT);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
    ASSERT_FALSE(is_victory_shown(&session->victory));
    ASSERT_TRUE(
        update_game_session_timer(game_port, session, TEST_COMPLETED_AT));
    ASSERT_TRUE(is_victory_shown(&session->victory));
    ASSERT_EQ_INT(session->victory.selected, VICTORY_OPTION_NEW_GAME);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_FALSE(session->timer.running);
    ASSERT_FALSE(session->timer.paused);
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 10);
}

static void test_update_game_session_timer_completion_opens_victory(void)
{
    run_session_test(check_completion_opens_victory);
}

static void check_victory_opens_once(const GamePort* game_port,
                                     GameSession* session)
{
    complete_session(game_port, session);
    press(game_port, session, TEST_DOWN_KEY, TEST_COMPLETED_AT);
    VictoryState victory = session->victory;
    ASSERT_EQ_INT(victory.selected, VICTORY_OPTION_MAIN_MENU);
    ASSERT_FALSE(
        update_game_session_timer(game_port, session, TEST_COMPLETED_AT + 1));
    ASSERT_FALSE(update_game_session_timer(game_port, session,
                                           TEST_COMPLETED_AT + 1000));
    ASSERT_TRUE(victory_states_match(&session->victory, &victory));
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 10);
    ASSERT_TRUE(
        summary_line_is(session, VICTORY_SUMMARY_TIME, "Time:       00:10"));
}

static void test_update_game_session_timer_victory_opens_once(void)
{
    run_session_test(check_victory_opens_once);
}

static void check_paused_game_opens_no_victory(const GamePort* game_port,
                                               GameSession* session)
{
    solve_session(game_port, session, TEST_START_TIME);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
    press(game_port, session, 'P', TEST_START_TIME + 5);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_MENU);
    ASSERT_FALSE(update_game_session_timer(game_port, session, 400));
    ASSERT_FALSE(is_victory_shown(&session->victory));
    press(game_port, session, TEST_ESCAPE, 600);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_FALSE(is_victory_shown(&session->victory));
    ASSERT_TRUE(update_game_session_timer(game_port, session, 602));
    ASSERT_TRUE(is_victory_shown(&session->victory));
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_TRUE(
        summary_line_is(session, VICTORY_SUMMARY_TIME, "Time:       00:07"));
}

static void test_update_game_session_timer_paused_game_opens_no_victory(void)
{
    run_session_test(check_paused_game_opens_no_victory);
}

static const char* difficulty_line_of(int difficulty)
{
    switch (difficulty)
    {
        case DIFFICULTY_EASY:
            return "Difficulty: Easy";
        case DIFFICULTY_MEDIUM:
            return "Difficulty: Medium";
        default:
            return "Difficulty: Hard";
    }
}

static void check_victory_summary_for(int difficulty)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    game_port->difficulty = difficulty;
    GameSession session;
    if (!start_session(game_port, &session, TEST_START_TIME))
    {
        destroy_game_port(game_port);
        ASSERT_TRUE(false);
    }
    solve_session(game_port, &session, TEST_START_TIME + 8);
    bool completed =
        update_game_session_timer(game_port, &session, TEST_START_TIME + 65);
    VictoryState victory = session.victory;
    int32_t recorded     = session.timer.elapsed_seconds;
    int moves            = moves_of(game_port, &session);
    char expected_time[TETONOR_ELAPSED_TEXT_SIZE];
    bool formatted =
        format_elapsed_time(recorded, expected_time, sizeof expected_time);
    end_game_session(game_port, &session);
    destroy_game_port(game_port);

    char time_line[VICTORY_SUMMARY_LINE_SIZE + VICTORY_SUMMARY_LABEL_W];
    char moves_line[VICTORY_SUMMARY_LINE_SIZE + VICTORY_SUMMARY_LABEL_W];
    snprintf(time_line, sizeof time_line, "%-*s%s", VICTORY_SUMMARY_LABEL_W,
             "Time:", expected_time);
    snprintf(moves_line, sizeof moves_line, "%-*s%d", VICTORY_SUMMARY_LABEL_W,
             "Moves:", moves);
    ASSERT_TRUE(completed);
    ASSERT_TRUE(formatted);
    ASSERT_EQ_INT(recorded, 65);
    ASSERT_EQ_INT(moves, TEST_SOLVE_MOVES);
    ASSERT_EQ_INT(
        strcmp(victory.summary.lines[VICTORY_SUMMARY_TIME], time_line), 0);
    ASSERT_EQ_INT(strcmp(victory.summary.lines[VICTORY_SUMMARY_TIME],
                         "Time:       01:05"),
                  0);
    ASSERT_EQ_INT(
        strcmp(victory.summary.lines[VICTORY_SUMMARY_MOVES], moves_line), 0);
    ASSERT_EQ_INT(strcmp(victory.summary.lines[VICTORY_SUMMARY_DIFFICULTY],
                         difficulty_line_of(difficulty)),
                  0);
}

static void test_victory_summary_matches_the_finished_game(void)
{
    check_victory_summary_for(DIFFICULTY_EASY);
    check_victory_summary_for(DIFFICULTY_MEDIUM);
    check_victory_summary_for(DIFFICULTY_HARD);
}

static const int keys_the_victory_screen_ignores[] = {'0',
                                                      '1',
                                                      '5',
                                                      '9',
                                                      TEST_LEFT_KEY,
                                                      TEST_RIGHT_KEY,
                                                      '+',
                                                      TEST_PAD_PLUS_KEY,
                                                      'x',
                                                      'X',
                                                      '*',
                                                      TEST_PAD_STAR_KEY,
                                                      '\b',
                                                      TEST_DELETE_KEY,
                                                      TEST_BACKSPACE_KEY,
                                                      TEST_PAD_ZERO_KEY,
                                                      TEST_PAD_ENTER_KEY,
                                                      'u',
                                                      'U',
                                                      'p',
                                                      'P',
                                                      'y',
                                                      'Y',
                                                      's',
                                                      'S'};

static void assert_victory_ignores(const GamePort* game_port,
                                   GameSession* session, const int* keys,
                                   size_t count)
{
    Board board_before   = session->board;
    GameTimer timer      = session->timer;
    PauseState pause     = session->pause;
    VictoryState victory = session->victory;
    int moves            = moves_of(game_port, session);
    for (size_t i = 0; i < count; i++)
    {
        GameSessionOutcome outcome =
            press(game_port, session, keys[i], TEST_START_TIME + 1000);
        ASSERT_EQ_INT(outcome, GAME_SESSION_CONTINUE);
        ASSERT_TRUE(boards_match(&session->board, &board_before));
        ASSERT_EQ_INT(moves_of(game_port, session), moves);
        ASSERT_TRUE(timers_match(&session->timer, &timer));
        ASSERT_TRUE(pause_states_match(&session->pause, &pause));
        ASSERT_TRUE(victory_states_match(&session->victory, &victory));
        ASSERT_FALSE(session->undo_notice);
    }
}

static void
check_victory_ignores_board_and_pause_keys(const GamePort* game_port,
                                           GameSession* session)
{
    complete_session(game_port, session);
    assert_victory_ignores(game_port, session, keys_the_victory_screen_ignores,
                           sizeof keys_the_victory_screen_ignores /
                               sizeof keys_the_victory_screen_ignores[0]);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_FALSE(session->timer.running);
    ASSERT_FALSE(session->timer.paused);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
    ASSERT_EQ_INT(fake_generate_calls, 1);

    press(game_port, session, TEST_DOWN_KEY, TEST_START_TIME + 1000);
    ASSERT_EQ_INT(session->victory.selected, VICTORY_OPTION_MAIN_MENU);
    assert_victory_ignores(game_port, session, keys_the_victory_screen_ignores,
                           sizeof keys_the_victory_screen_ignores /
                               sizeof keys_the_victory_screen_ignores[0]);
    ASSERT_EQ_INT(session->victory.selected, VICTORY_OPTION_MAIN_MENU);
}

static void
test_handle_game_session_key_victory_ignores_board_and_pause_keys(void)
{
    run_session_test(check_victory_ignores_board_and_pause_keys);
}

static void check_victory_navigation(const GamePort* game_port,
                                     GameSession* session)
{
    complete_session(game_port, session);
    Board board_before = session->board;
    ASSERT_EQ_INT(press(game_port, session, TEST_UP_KEY, TEST_COMPLETED_AT),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->victory.selected, VICTORY_OPTION_NEW_GAME);
    ASSERT_EQ_INT(press(game_port, session, TEST_DOWN_KEY, TEST_COMPLETED_AT),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->victory.selected, VICTORY_OPTION_MAIN_MENU);
    ASSERT_EQ_INT(press(game_port, session, TEST_DOWN_KEY, TEST_COMPLETED_AT),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->victory.selected, VICTORY_OPTION_MAIN_MENU);
    ASSERT_EQ_INT(press(game_port, session, TEST_UP_KEY, TEST_COMPLETED_AT),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->victory.selected, VICTORY_OPTION_NEW_GAME);
    ASSERT_TRUE(is_victory_shown(&session->victory));
    ASSERT_TRUE(boards_match(&session->board, &board_before));
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
}

static void test_handle_game_session_key_victory_navigation(void)
{
    run_session_test(check_victory_navigation);
}

/* Presses key on the victory screen of a completed game and checks that a
 * fresh game started without asking. */
static void assert_victory_key_starts_new_game(const GamePort* game_port,
                                               GameSession* session, int key)
{
    complete_session(game_port, session);
    const NumberGeneration* old = session->puzzle;
    ASSERT_EQ_INT(moves_of(game_port, session), TEST_SOLVE_MOVES);
    ASSERT_EQ_INT(press(game_port, session, key, TEST_RESTART_AT),
                  GAME_SESSION_RESTARTED);
    ASSERT_EQ_INT(fake_generate_calls, 2);
    ASSERT_EQ_INT(fake_free_calls, 1);
    ASSERT_TRUE(session->puzzle != old);
    ASSERT_TRUE(session_is_fresh(session, TEST_RESTART_AT));
    ASSERT_TRUE(session_has_no_victory_or_moves(game_port, session));
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_EQ_INT(session->pause.pending, GAME_ACTION_NONE);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_PENDING);
    ASSERT_TRUE(session->timer.running);
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 0);
}

static void check_victory_lower_n_starts_new_game(const GamePort* game_port,
                                                  GameSession* session)
{
    assert_victory_key_starts_new_game(game_port, session, 'n');
}

static void check_victory_upper_n_starts_new_game(const GamePort* game_port,
                                                  GameSession* session)
{
    assert_victory_key_starts_new_game(game_port, session, 'N');
}

static void check_victory_enter_starts_new_game(const GamePort* game_port,
                                                GameSession* session)
{
    assert_victory_key_starts_new_game(game_port, session, TEST_ENTER_KEY);
}

static void check_victory_space_starts_new_game(const GamePort* game_port,
                                                GameSession* session)
{
    assert_victory_key_starts_new_game(game_port, session, ' ');
}

static void test_handle_game_session_key_victory_new_game(void)
{
    run_session_test(check_victory_lower_n_starts_new_game);
    run_session_test(check_victory_upper_n_starts_new_game);
    run_session_test(check_victory_enter_starts_new_game);
    run_session_test(check_victory_space_starts_new_game);
}

static void assert_victory_key_leaves(const GamePort* game_port,
                                      GameSession* session, const int* keys,
                                      size_t count)
{
    complete_session(game_port, session);
    GameSessionOutcome outcome = GAME_SESSION_CONTINUE;
    for (size_t i = 0; i < count; i++)
    {
        outcome = press(game_port, session, keys[i], TEST_RESTART_AT);
    }
    ASSERT_EQ_INT(outcome, GAME_SESSION_LEAVE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_EQ_INT(session->pause.pending, GAME_ACTION_NONE);
    ASSERT_EQ_INT(fake_generate_calls, 1);
    ASSERT_EQ_INT(fake_free_calls, 0);
}

static void check_victory_lower_m_leaves(const GamePort* game_port,
                                         GameSession* session)
{
    const int keys[] = {'m'};
    assert_victory_key_leaves(game_port, session, keys, 1);
}

static void check_victory_upper_m_leaves(const GamePort* game_port,
                                         GameSession* session)
{
    const int keys[] = {'M'};
    assert_victory_key_leaves(game_port, session, keys, 1);
}

static void check_victory_escape_leaves(const GamePort* game_port,
                                        GameSession* session)
{
    const int keys[] = {TEST_ESCAPE};
    assert_victory_key_leaves(game_port, session, keys, 1);
}

static void check_victory_down_enter_leaves(const GamePort* game_port,
                                            GameSession* session)
{
    const int keys[] = {TEST_DOWN_KEY, TEST_ENTER_KEY};
    assert_victory_key_leaves(game_port, session, keys, 2);
}

static void check_victory_down_space_leaves(const GamePort* game_port,
                                            GameSession* session)
{
    const int keys[] = {TEST_DOWN_KEY, ' '};
    assert_victory_key_leaves(game_port, session, keys, 2);
}

static void test_handle_game_session_key_victory_main_menu(void)
{
    run_session_test(check_victory_lower_m_leaves);
    run_session_test(check_victory_upper_m_leaves);
    run_session_test(check_victory_escape_leaves);
    run_session_test(check_victory_down_enter_leaves);
    run_session_test(check_victory_down_space_leaves);
}

static void assert_victory_new_game_failure(const GamePort* game_port,
                                            GameSession* session,
                                            const int* keys, size_t count)
{
    complete_session(game_port, session);
    Board board_before             = session->board;
    VictoryState victory           = session->victory;
    GameTimer timer                = session->timer;
    const NumberGeneration* puzzle = session->puzzle;
    fake_generate_fails            = true;
    GameSessionOutcome outcome     = GAME_SESSION_RESTARTED;
    for (size_t i = 0; i < count; i++)
    {
        outcome = press(game_port, session, keys[i], TEST_RESTART_AT);
    }
    ASSERT_EQ_INT(outcome, GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(fake_generate_calls, 2);
    ASSERT_EQ_INT(fake_free_calls, 0);
    ASSERT_TRUE(session->puzzle == puzzle);
    ASSERT_TRUE(is_victory_shown(&session->victory));
    ASSERT_TRUE(victory_states_match(&session->victory, &victory));
    ASSERT_TRUE(boards_match(&session->board, &board_before));
    ASSERT_TRUE(timers_match(&session->timer, &timer));
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);

    fake_generate_fails = false;
    ASSERT_EQ_INT(press(game_port, session, 'n', TEST_RESTART_AT + 1),
                  GAME_SESSION_RESTARTED);
    ASSERT_TRUE(session_is_fresh(session, TEST_RESTART_AT + 1));
    ASSERT_TRUE(session_has_no_victory_or_moves(game_port, session));
}

static void check_victory_new_game_key_failure(const GamePort* game_port,
                                               GameSession* session)
{
    const int keys[] = {'N'};
    assert_victory_new_game_failure(game_port, session, keys, 1);
}

static void check_victory_new_game_select_failure(const GamePort* game_port,
                                                  GameSession* session)
{
    const int keys[] = {TEST_ENTER_KEY};
    assert_victory_new_game_failure(game_port, session, keys, 1);
}

static void
test_handle_game_session_key_victory_new_game_failure_keeps_summary(void)
{
    run_session_test(check_victory_new_game_key_failure);
    run_session_test(check_victory_new_game_select_failure);
}

static void check_victory_unknown_keys_change_nothing(const GamePort* game_port,
                                                      GameSession* session)
{
    complete_session(game_port, session);
    assert_victory_ignores(game_port, session, keys_nobody_binds,
                           sizeof keys_nobody_binds /
                               sizeof keys_nobody_binds[0]);
    ASSERT_TRUE(is_victory_shown(&session->victory));
}

static void
test_handle_game_session_key_victory_unknown_keys_change_nothing(void)
{
    run_session_test(check_victory_unknown_keys_change_nothing);
}

static void check_victory_again_after_new_game(const GamePort* game_port,
                                               GameSession* session)
{
    complete_session(game_port, session);
    ASSERT_EQ_INT(press(game_port, session, 'N', 200), GAME_SESSION_RESTARTED);
    ASSERT_FALSE(is_victory_shown(&session->victory));
    ASSERT_FALSE(update_game_session_timer(game_port, session, 201));
    ASSERT_FALSE(is_victory_shown(&session->victory));

    solve_session(game_port, session, 205);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(update_game_session_timer(game_port, session, 207));
    ASSERT_TRUE(is_victory_shown(&session->victory));
    ASSERT_EQ_INT(session->victory.selected, VICTORY_OPTION_NEW_GAME);
    ASSERT_TRUE(
        summary_line_is(session, VICTORY_SUMMARY_TIME, "Time:       00:07"));
    ASSERT_TRUE(
        summary_line_is(session, VICTORY_SUMMARY_MOVES, "Moves:      5"));
    ASSERT_EQ_INT(moves_of(game_port, session), TEST_SOLVE_MOVES);
    ASSERT_FALSE(update_game_session_timer(game_port, session, 208));
}

static void test_victory_again_after_new_game(void)
{
    run_session_test(check_victory_again_after_new_game);
}

static void check_moves_through_the_session(const GamePort* game_port,
                                            GameSession* session)
{
    ASSERT_EQ_INT(moves_of(game_port, session), TEST_NO_EDITS_YET);
    make_edit(game_port, session);
    press(game_port, session, TEST_RIGHT_KEY, TEST_START_TIME);
    press(game_port, session, TEST_OTHER_DIGIT, TEST_START_TIME);
    ASSERT_EQ_INT(moves_of(game_port, session), 2);
    press(game_port, session, 'u', TEST_START_TIME);
    ASSERT_FALSE(session->undo_notice);
    ASSERT_EQ_INT(session->board.history.count, 1);
    ASSERT_EQ_INT(moves_of(game_port, session), 2);
    press(game_port, session, 'p', TEST_START_TIME + 3);
    press(game_port, session, TEST_ESCAPE, TEST_START_TIME + 4);
    ASSERT_EQ_INT(moves_of(game_port, session), 2);
    press(game_port, session, TEST_OTHER_DIGIT, TEST_START_TIME + 5);
    ASSERT_EQ_INT(moves_of(game_port, session), 3);
}

static void test_handle_game_session_key_undo_keeps_the_move_count(void)
{
    run_session_test(check_moves_through_the_session);
}

static void check_restart_button_resets_moves(const GamePort* game_port,
                                              GameSession* session)
{
    make_edit(game_port, session);
    press(game_port, session, TEST_RIGHT_KEY, TEST_START_TIME);
    press(game_port, session, TEST_OTHER_DIGIT, TEST_START_TIME);
    ASSERT_EQ_INT(moves_of(game_port, session), 2);
    press(game_port, session, TEST_DOWN_KEY, TEST_START_TIME);
    press(game_port, session, TEST_DOWN_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(session->board.focus, BOARD_FOCUS_RESTART);
    ASSERT_EQ_INT(press(game_port, session, TEST_ENTER_KEY, 300),
                  GAME_SESSION_RESTARTED);
    ASSERT_TRUE(session_is_fresh(session, 300));
    ASSERT_TRUE(session_has_no_victory_or_moves(game_port, session));
}

static void check_new_game_prompt_resets_moves(const GamePort* game_port,
                                               GameSession* session)
{
    make_edit(game_port, session);
    press(game_port, session, TEST_RIGHT_KEY, TEST_START_TIME);
    press(game_port, session, TEST_OTHER_DIGIT, TEST_START_TIME);
    ASSERT_EQ_INT(moves_of(game_port, session), 2);
    press(game_port, session, 'N', TEST_START_TIME);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PROMPT);
    ASSERT_EQ_INT(moves_of(game_port, session), 2);
    ASSERT_EQ_INT(press(game_port, session, 'y', 301), GAME_SESSION_RESTARTED);
    ASSERT_TRUE(session_is_fresh(session, 301));
    ASSERT_TRUE(session_has_no_victory_or_moves(game_port, session));
}

static void check_failed_new_game_keeps_moves(const GamePort* game_port,
                                              GameSession* session)
{
    make_edit(game_port, session);
    press(game_port, session, TEST_RIGHT_KEY, TEST_START_TIME);
    press(game_port, session, TEST_OTHER_DIGIT, TEST_START_TIME);
    fake_generate_fails = true;
    press(game_port, session, 'N', TEST_START_TIME);
    ASSERT_EQ_INT(press(game_port, session, 'y', TEST_START_TIME),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(moves_of(game_port, session), 2);
    ASSERT_FALSE(is_victory_shown(&session->victory));
}

static void test_restart_button_and_n_reset_moves_and_victory(void)
{
    run_session_test(check_restart_button_resets_moves);
    run_session_test(check_new_game_prompt_resets_moves);
    run_session_test(check_failed_new_game_keeps_moves);
}

#define TEST_THEME_OPENED_AT (TEST_START_TIME + 10)
#define TEST_THEME_CLOSED_AT (TEST_THEME_OPENED_AT + 30)
#define TEST_AFTER_THEME_AT (TEST_THEME_CLOSED_AT + 2)
#define TEST_FAR_THEME 99

static const int theme_wrong_keys[] = {TEST_UP_KEY, TEST_DELETE_KEY, '8',
                                       TEST_RIGHT_KEY, TEST_ENTER_KEY};

static GameSessionOutcome choose_next_theme(const GamePort* game_port,
                                            GameSession* session, time_t now)
{
    press(game_port, session, TEST_DOWN_KEY, now);
    return press(game_port, session, TEST_ENTER_KEY, now);
}

static void test_init_game_session_uses_default_theme(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    GameSession session;
    bool started       = start_session(game_port, &session, TEST_START_TIME);
    PaletteTheme theme = session.theme;
    bool shown         = is_theme_screen_shown(&session.pause);
    ThemeMenuStatus status = session.theme_menu.status;
    if (started)
    {
        end_game_session(game_port, &session);
    }
    destroy_game_port(game_port);
    ASSERT_TRUE(started);
    ASSERT_EQ_INT(theme, TETONOR_DEFAULT_PALETTE_THEME);
    ASSERT_FALSE(shown);
    ASSERT_EQ_INT(status, THEME_MENU_OPEN);
}

static void check_set_theme_sets_and_defaults(const GamePort* game_port,
                                              GameSession* session)
{
    make_edit(game_port, session);
    Board board      = session->board;
    GameTimer timer  = session->timer;
    PauseState pause = session->pause;
    set_game_session_theme(session, PALETTE_THEME_LIGHT);
    ASSERT_EQ_INT(session->theme, PALETTE_THEME_LIGHT);
    set_game_session_theme(session, -1);
    ASSERT_EQ_INT(session->theme, TETONOR_DEFAULT_PALETTE_THEME);
    set_game_session_theme(session, PALETTE_THEME_LIGHT);
    set_game_session_theme(session, PALETTE_THEME_COUNT);
    ASSERT_EQ_INT(session->theme, TETONOR_DEFAULT_PALETTE_THEME);
    set_game_session_theme(session, PALETTE_THEME_LIGHT);
    set_game_session_theme(session, TEST_FAR_THEME);
    ASSERT_EQ_INT(session->theme, TETONOR_DEFAULT_PALETTE_THEME);
    set_game_session_theme(session, PALETTE_THEME_LIGHT);
    ASSERT_EQ_INT(session->theme, PALETTE_THEME_LIGHT);
    ASSERT_TRUE(boards_match(&session->board, &board));
    ASSERT_TRUE(timers_match(&session->timer, &timer));
    ASSERT_TRUE(pause_states_match(&session->pause, &pause));
    ASSERT_EQ_INT(fake_generate_calls, 1);
}

static void test_set_game_session_theme_sets_and_defaults(void)
{
    run_session_test(check_set_theme_sets_and_defaults);
}

static void check_t_opens_theme_screen_and_pauses(const GamePort* game_port,
                                                  GameSession* session)
{
    const int keys[] = {'t', 'T'};
    set_game_session_theme(session, PALETTE_THEME_LIGHT);
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(press(game_port, session, keys[i], TEST_THEME_OPENED_AT),
                      GAME_SESSION_CONTINUE);
        ASSERT_TRUE(is_theme_screen_shown(&session->pause));
        ASSERT_EQ_INT(session->theme_menu.active, PALETTE_THEME_LIGHT);
        ASSERT_EQ_INT(session->theme_menu.selected, PALETTE_THEME_LIGHT);
        ASSERT_EQ_INT(session->theme_menu.status, THEME_MENU_OPEN);
        ASSERT_TRUE(session->timer.paused);
        ASSERT_EQ_INT(
            press(game_port, session, TETONOR_KEY_ESCAPE, TEST_THEME_OPENED_AT),
            GAME_SESSION_CONTINUE);
        ASSERT_FALSE(is_theme_screen_shown(&session->pause));
        ASSERT_FALSE(session->timer.paused);
    }
}

static void test_handle_game_session_key_t_opens_theme_screen_and_pauses(void)
{
    run_session_test(check_t_opens_theme_screen_and_pauses);
}

static void check_theme_change_keeps_the_game(const GamePort* game_port,
                                              GameSession* session)
{
    make_edit(game_port, session);
    press(game_port, session, TEST_RIGHT_KEY, TEST_START_TIME);
    press(game_port, session, TEST_OTHER_DIGIT, TEST_START_TIME);
    ASSERT_FALSE(
        update_game_session_timer(game_port, session, TEST_THEME_OPENED_AT));
    int32_t elapsed = session->timer.elapsed_seconds;
    ASSERT_EQ_INT(elapsed, TEST_THEME_OPENED_AT - TEST_START_TIME);
    Board before       = session->board;
    int history        = session->board.history.count;
    int moves          = moves_of(game_port, session);
    BoardResult result = session->board.result;
    ASSERT_TRUE(history > 0);

    press(game_port, session, 'T', TEST_THEME_OPENED_AT);
    ASSERT_TRUE(is_theme_screen_shown(&session->pause));
    ASSERT_EQ_INT(choose_next_theme(game_port, session, TEST_THEME_CLOSED_AT),
                  GAME_SESSION_THEME_CHANGED);

    ASSERT_EQ_INT(session->theme, PALETTE_THEME_LIGHT);
    ASSERT_FALSE(is_theme_screen_shown(&session->pause));
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_TRUE(boards_match(&session->board, &before));
    ASSERT_EQ_INT(session->board.history.count, history);
    ASSERT_EQ_INT(moves_of(game_port, session), moves);
    ASSERT_EQ_INT(session->board.result, result);
    ASSERT_EQ_INT(session->timer.elapsed_seconds, elapsed);
    ASSERT_TRUE(session->timer.running);
    ASSERT_FALSE(session->timer.paused);
    ASSERT_FALSE(is_victory_shown(&session->victory));
    ASSERT_EQ_INT(fake_generate_calls, 1);
    ASSERT_EQ_INT(fake_free_calls, 0);

    ASSERT_FALSE(
        update_game_session_timer(game_port, session, TEST_AFTER_THEME_AT));
    ASSERT_EQ_INT(session->timer.elapsed_seconds,
                  elapsed + (TEST_AFTER_THEME_AT - TEST_THEME_CLOSED_AT));
}

static void test_theme_change_keeps_board_history_result_and_timer(void)
{
    run_session_test(check_theme_change_keeps_the_game);
}

static void check_theme_change_on_wrong_board(const GamePort* game_port,
                                              GameSession* session)
{
    fill_without_confirming(game_port, session, TEST_START_TIME);
    press_all(game_port, session, theme_wrong_keys,
              sizeof theme_wrong_keys / sizeof theme_wrong_keys[0],
              TEST_START_TIME);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_WRONG);
    Board before = session->board;

    press(game_port, session, 't', TEST_THEME_OPENED_AT);
    ASSERT_EQ_INT(choose_next_theme(game_port, session, TEST_THEME_CLOSED_AT),
                  GAME_SESSION_THEME_CHANGED);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_WRONG);
    ASSERT_TRUE(boards_match(&session->board, &before));

    ASSERT_EQ_INT(press(game_port, session, 'u', TEST_AFTER_THEME_AT),
                  GAME_SESSION_CONTINUE);
    bool undone = game_port->undo_board_move_f(&before);
    ASSERT_EQ_INT(session->undo_notice, !undone);
    ASSERT_TRUE(boards_match(&session->board, &before));
}

static void test_theme_change_on_wrong_board_keeps_verdict(void)
{
    run_session_test(check_theme_change_on_wrong_board);
}

static void check_theme_change_on_solved_board(const GamePort* game_port,
                                               GameSession* session)
{
    solve_session(game_port, session, TEST_START_TIME);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
    Board before = session->board;
    press(game_port, session, 'T', TEST_START_TIME);
    ASSERT_TRUE(is_theme_screen_shown(&session->pause));
    ASSERT_EQ_INT(choose_next_theme(game_port, session, TEST_START_TIME),
                  GAME_SESSION_THEME_CHANGED);
    ASSERT_TRUE(boards_match(&session->board, &before));
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
    ASSERT_FALSE(is_victory_shown(&session->victory));
    ASSERT_TRUE(
        update_game_session_timer(game_port, session, TEST_COMPLETED_AT));
    ASSERT_TRUE(is_victory_shown(&session->victory));
}

static void test_theme_change_on_solved_board_still_completes(void)
{
    run_session_test(check_theme_change_on_solved_board);
}

static void check_theme_select_same_theme(const GamePort* game_port,
                                          GameSession* session)
{
    make_edit(game_port, session);
    Board before = session->board;
    press(game_port, session, 'T', TEST_START_TIME);
    ASSERT_EQ_INT(press(game_port, session, TEST_ENTER_KEY, TEST_START_TIME),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->theme, TETONOR_DEFAULT_PALETTE_THEME);
    ASSERT_FALSE(is_theme_screen_shown(&session->pause));
    ASSERT_FALSE(session->timer.paused);
    ASSERT_TRUE(boards_match(&session->board, &before));

    set_game_session_theme(session, PALETTE_THEME_LIGHT);
    press(game_port, session, 'T', TEST_START_TIME);
    ASSERT_EQ_INT(press(game_port, session, ' ', TEST_START_TIME),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->theme, PALETTE_THEME_LIGHT);
    ASSERT_FALSE(is_theme_screen_shown(&session->pause));
}

static void test_theme_select_same_theme_is_continue(void)
{
    run_session_test(check_theme_select_same_theme);
}

static void check_theme_screen_back_keeps_theme(const GamePort* game_port,
                                                GameSession* session)
{
    const int back_keys[] = {TETONOR_KEY_ESCAPE, 't', 'T'};
    for (size_t i = 0; i < sizeof back_keys / sizeof back_keys[0]; i++)
    {
        make_edit(game_port, session);
        Board before = session->board;
        press(game_port, session, 'T', TEST_START_TIME);
        press(game_port, session, TEST_DOWN_KEY, TEST_START_TIME);
        ASSERT_EQ_INT(press(game_port, session, back_keys[i], TEST_START_TIME),
                      GAME_SESSION_CONTINUE);
        ASSERT_EQ_INT(session->theme, TETONOR_DEFAULT_PALETTE_THEME);
        ASSERT_FALSE(is_theme_screen_shown(&session->pause));
        ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
        ASSERT_FALSE(session->timer.paused);
        ASSERT_TRUE(boards_match(&session->board, &before));
        press(game_port, session, 'u', TEST_START_TIME);
    }
}

static void test_theme_screen_back_keeps_theme(void)
{
    run_session_test(check_theme_screen_back_keeps_theme);
}

static void
check_theme_screen_back_does_not_leave_the_game(const GamePort* game_port,
                                                GameSession* session)
{
    press(game_port, session, 'T', TEST_START_TIME);
    ASSERT_EQ_INT(
        press(game_port, session, TETONOR_KEY_ESCAPE, TEST_START_TIME),
        GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(fake_free_calls, 0);
}

static void test_theme_screen_escape_never_leaves_the_game(void)
{
    run_session_test(check_theme_screen_back_does_not_leave_the_game);
}

static void check_theme_screen_ignores_game_keys(const GamePort* game_port,
                                                 GameSession* session)
{
    make_edit(game_port, session);
    const int keys[] = {'0',
                        TEST_TYPED_DIGIT,
                        '9',
                        'u',
                        'U',
                        'p',
                        'P',
                        'n',
                        'N',
                        'm',
                        'M',
                        's',
                        'y',
                        '+',
                        'x',
                        TEST_BACKSPACE_KEY,
                        TEST_DELETE_KEY,
                        TEST_LEFT_KEY,
                        TEST_RIGHT_KEY,
                        TEST_RESIZE_KEY,
                        0,
                        TEST_ERR_KEY};
    press(game_port, session, 'T', TEST_START_TIME);
    Board board      = session->board;
    GameTimer timer  = session->timer;
    PauseState pause = session->pause;
    ThemeMenu menu   = session->theme_menu;
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(press(game_port, session, keys[i], TEST_START_TIME),
                      GAME_SESSION_CONTINUE);
        ASSERT_TRUE(is_theme_screen_shown(&session->pause));
        ASSERT_TRUE(boards_match(&session->board, &board));
        ASSERT_TRUE(timers_match(&session->timer, &timer));
        ASSERT_TRUE(pause_states_match(&session->pause, &pause));
        ASSERT_EQ_INT(session->theme_menu.selected, menu.selected);
        ASSERT_EQ_INT(session->theme_menu.status, THEME_MENU_OPEN);
        ASSERT_FALSE(session->undo_notice);
    }
    ASSERT_EQ_INT(fake_generate_calls, 1);
    ASSERT_EQ_INT(fake_free_calls, 0);
}

static void test_theme_screen_ignores_game_keys(void)
{
    run_session_test(check_theme_screen_ignores_game_keys);
}

static void
check_theme_screen_navigation_stops_at_the_ends(const GamePort* game_port,
                                                GameSession* session)
{
    press(game_port, session, 'T', TEST_START_TIME);
    press(game_port, session, TEST_UP_KEY, TEST_START_TIME);
    ASSERT_EQ_INT(session->theme_menu.selected, PALETTE_THEME_CLASSIC);
    for (int step = 0; step < PALETTE_THEME_COUNT + 1; step++)
    {
        press(game_port, session, TEST_DOWN_KEY, TEST_START_TIME);
    }
    ASSERT_EQ_INT(session->theme_menu.selected, PALETTE_THEME_COUNT - 1);
    ASSERT_EQ_INT(session->theme, TETONOR_DEFAULT_PALETTE_THEME);
    ASSERT_TRUE(is_theme_screen_shown(&session->pause));
}

static void test_theme_screen_navigation_does_not_change_the_theme(void)
{
    run_session_test(check_theme_screen_navigation_stops_at_the_ends);
}

static void check_pause_menu_t_returns_to_menu(const GamePort* game_port,
                                               GameSession* session)
{
    press(game_port, session, 'P', TEST_START_TIME + 1);
    press(game_port, session, TEST_DOWN_KEY, TEST_START_TIME + 1);
    ASSERT_EQ_INT(session->pause.selected, PAUSE_OPTION_ABANDON);
    ASSERT_TRUE(session->timer.paused);
    int32_t elapsed = session->timer.elapsed_seconds;
    ASSERT_EQ_INT(press(game_port, session, 'T', TEST_THEME_OPENED_AT),
                  GAME_SESSION_CONTINUE);
    ASSERT_TRUE(is_theme_screen_shown(&session->pause));
    ASSERT_EQ_INT(session->theme_menu.active, TETONOR_DEFAULT_PALETTE_THEME);
    ASSERT_TRUE(session->timer.paused);
    ASSERT_EQ_INT(
        press(game_port, session, TETONOR_KEY_ESCAPE, TEST_THEME_CLOSED_AT),
        GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_MENU);
    ASSERT_EQ_INT(session->pause.selected, PAUSE_OPTION_ABANDON);
    ASSERT_TRUE(session->timer.paused);
    ASSERT_EQ_INT(session->timer.elapsed_seconds, elapsed);

    press(game_port, session, TEST_UP_KEY, TEST_THEME_CLOSED_AT);
    press(game_port, session, 'p', TEST_THEME_CLOSED_AT);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_FALSE(session->timer.paused);
    ASSERT_EQ_INT(session->timer.elapsed_seconds, elapsed);
}

static void test_pause_menu_t_opens_theme_and_returns_to_menu(void)
{
    run_session_test(check_pause_menu_t_returns_to_menu);
}

static void check_pause_menu_theme_change(const GamePort* game_port,
                                          GameSession* session)
{
    make_edit(game_port, session);
    Board before = session->board;
    press(game_port, session, 'P', TEST_START_TIME);
    press(game_port, session, 't', TEST_START_TIME);
    ASSERT_EQ_INT(choose_next_theme(game_port, session, TEST_THEME_CLOSED_AT),
                  GAME_SESSION_THEME_CHANGED);
    ASSERT_EQ_INT(session->theme, PALETTE_THEME_LIGHT);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_MENU);
    ASSERT_EQ_INT(session->pause.selected, PAUSE_OPTION_RESUME);
    ASSERT_TRUE(session->timer.paused);
    ASSERT_TRUE(boards_match(&session->board, &before));
    ASSERT_EQ_INT(
        press(game_port, session, TETONOR_KEY_ESCAPE, TEST_THEME_CLOSED_AT),
        GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_FALSE(session->timer.paused);
}

static void test_pause_menu_theme_change_returns_to_menu(void)
{
    run_session_test(check_pause_menu_theme_change);
}

static void
check_theme_screen_reopens_marking_the_new_theme(const GamePort* game_port,
                                                 GameSession* session)
{
    press(game_port, session, 'T', TEST_START_TIME);
    ASSERT_EQ_INT(choose_next_theme(game_port, session, TEST_START_TIME),
                  GAME_SESSION_THEME_CHANGED);
    press(game_port, session, 'T', TEST_START_TIME);
    ASSERT_EQ_INT(session->theme_menu.active, PALETTE_THEME_LIGHT);
    ASSERT_EQ_INT(session->theme_menu.selected, PALETTE_THEME_LIGHT);
    ASSERT_EQ_INT(session->theme_menu.status, THEME_MENU_OPEN);
}

static void test_theme_screen_reopens_marking_the_new_theme(void)
{
    run_session_test(check_theme_screen_reopens_marking_the_new_theme);
}

static void check_victory_ignores_t(const GamePort* game_port,
                                    GameSession* session)
{
    complete_session(game_port, session);
    VictoryState victory = session->victory;
    const int keys[]     = {'t', 'T'};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(press(game_port, session, keys[i], TEST_COMPLETED_AT),
                      GAME_SESSION_CONTINUE);
        ASSERT_TRUE(is_victory_shown(&session->victory));
        ASSERT_FALSE(is_theme_screen_shown(&session->pause));
        ASSERT_TRUE(victory_states_match(&session->victory, &victory));
        ASSERT_EQ_INT(session->theme, TETONOR_DEFAULT_PALETTE_THEME);
    }
}

static void check_prompt_ignores_t(const GamePort* game_port,
                                   GameSession* session)
{
    make_edit(game_port, session);
    press(game_port, session, 'N', TEST_START_TIME);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PROMPT);
    PauseState pause = session->pause;
    const int keys[] = {'t', 'T'};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(press(game_port, session, keys[i], TEST_START_TIME),
                      GAME_SESSION_CONTINUE);
        ASSERT_FALSE(is_theme_screen_shown(&session->pause));
        ASSERT_TRUE(pause_states_match(&session->pause, &pause));
    }
}

static void test_victory_and_prompt_ignore_t(void)
{
    run_session_test(check_victory_ignores_t);
    run_session_test(check_prompt_ignores_t);
}

static void check_restart_keeps_the_theme(const GamePort* game_port,
                                          GameSession* session)
{
    set_game_session_theme(session, PALETTE_THEME_LIGHT);
    make_edit(game_port, session);
    press(game_port, session, 'N', TEST_START_TIME);
    ASSERT_EQ_INT(press(game_port, session, 'y', TEST_RESTART_AT),
                  GAME_SESSION_RESTARTED);
    ASSERT_EQ_INT(session->theme, PALETTE_THEME_LIGHT);
    ASSERT_FALSE(is_theme_screen_shown(&session->pause));

    press(game_port, session, TEST_DOWN_KEY, TEST_RESTART_AT);
    press(game_port, session, TEST_DOWN_KEY, TEST_RESTART_AT);
    ASSERT_EQ_INT(session->board.focus, BOARD_FOCUS_RESTART);
    ASSERT_EQ_INT(press(game_port, session, TEST_ENTER_KEY, TEST_RESTART_AT),
                  GAME_SESSION_RESTARTED);
    ASSERT_EQ_INT(session->theme, PALETTE_THEME_LIGHT);
}

static void test_restart_keeps_session_theme(void)
{
    run_session_test(check_restart_keeps_the_theme);
}

static void check_failed_new_game_keeps_the_theme(const GamePort* game_port,
                                                  GameSession* session)
{
    make_edit(game_port, session);
    press(game_port, session, 'T', TEST_START_TIME);
    choose_next_theme(game_port, session, TEST_START_TIME);
    fake_generate_fails = true;
    press(game_port, session, 'N', TEST_START_TIME);
    ASSERT_EQ_INT(press(game_port, session, 'y', TEST_START_TIME),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->theme, PALETTE_THEME_LIGHT);
}

static void test_failed_new_game_keeps_session_theme(void)
{
    run_session_test(check_failed_new_game_keeps_the_theme);
}

#define TEST_PAUSED_AT (TEST_START_TIME + 30)
#define TEST_SAVE_KEY 's'
#define TEST_SAVE_KEY_UPPER 'S'
#define TEST_RESTORED_CURSOR 1
#define TEST_CONTINUED_AT ((time_t)110)
#define TEST_CONTINUED_COMPLETED_AT ((time_t)120)
#define TEST_CONTINUED_EXTRA_SECONDS 20
#define TEST_CONTINUED_EXTRA_MOVES 2
#define TEST_SECOND_HIDDEN_SLOT_VALUE '3'
#define TEST_LAST_HIDDEN_SLOT_VALUE '9'
#define TEST_GARBAGE_BYTE 0x5a

static NumberGeneration* fake_generate_save_puzzle(int difficulty)
{
    fake_generate_calls++;
    fake_generate_difficulty = difficulty;
    if (fake_generate_fails)
    {
        return NULL;
    }
    NumberGeneration* puzzle = malloc(sizeof *puzzle);
    if (puzzle != NULL)
    {
        fake_alloc_count++;
        build_save_fixture_puzzle(puzzle, difficulty);
    }
    return puzzle;
}

/* A port whose puzzles are valid for saving at the given difficulty. */
static GamePort* create_save_port(int difficulty)
{
    GamePort* game_port = create_fake_port();
    if (game_port != NULL)
    {
        game_port->generate_random_numbers_f = &fake_generate_save_puzzle;
        game_port->difficulty                = difficulty;
    }
    return game_port;
}

static void run_save_session_test(int difficulty,
                                  void (*body)(const GamePort*, GameSession*))
{
    GamePort* game_port = create_save_port(difficulty);
    ASSERT_NOT_NULL(game_port);
    GameSession session;
    if (!start_session(game_port, &session, TEST_START_TIME))
    {
        destroy_game_port(game_port);
        ASSERT_TRUE(false);
    }
    body(game_port, &session);
    end_game_session(game_port, &session);
    destroy_game_port(game_port);
    ASSERT_EQ_INT(fake_free_calls, fake_alloc_count);
}

/* A session continued from the golden save. The restored puzzle is not made
 * by the fake generator, so it is one release more than the allocations. */
static void run_continued_session_test(void (*body)(const GamePort*,
                                                    GameSession*))
{
    GamePort* game_port = create_save_port(DIFFICULTY_EASY);
    ASSERT_NOT_NULL(game_port);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    GameSession session;
    if (!init_game_session_from_save(game_port, &session, &saved, &board_codes,
                                     &menu_codes, TEST_START_TIME))
    {
        destroy_game_port(game_port);
        ASSERT_TRUE(false);
    }
    body(game_port, &session);
    end_game_session(game_port, &session);
    destroy_game_port(game_port);
    ASSERT_EQ_INT(fake_free_calls, fake_alloc_count + 1);
}

static void check_s_in_the_pause_menu_requests_a_save(const GamePort* game_port,
                                                      GameSession* session)
{
    const int keys[] = {TEST_SAVE_KEY, TEST_SAVE_KEY_UPPER};
    make_edit(game_port, session);
    press(game_port, session, 'p', TEST_PAUSED_AT);
    ASSERT_TRUE(session->timer.paused);
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        Board board_before = session->board;
        GameTimer timer    = session->timer;
        ASSERT_EQ_INT(press(game_port, session, keys[i], TEST_PAUSED_AT + 500),
                      GAME_SESSION_SAVE);
        ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_MENU);
        ASSERT_EQ_INT(session->pause.selected, PAUSE_OPTION_RESUME);
        ASSERT_TRUE(boards_match(&session->board, &board_before));
        ASSERT_TRUE(timers_match(&session->timer, &timer));
        ASSERT_TRUE(session->timer.paused);
        ASSERT_FALSE(session->undo_notice);
    }
}

static void test_handle_game_session_key_s_in_the_pause_menu_asks_to_save(void)
{
    run_save_session_test(DIFFICULTY_EASY,
                          check_s_in_the_pause_menu_requests_a_save);
}

static void
check_save_request_keeps_the_menu_selection(const GamePort* game_port,
                                            GameSession* session)
{
    press(game_port, session, 'p', TEST_PAUSED_AT);
    press(game_port, session, TEST_DOWN_KEY, TEST_PAUSED_AT);
    ASSERT_EQ_INT(session->pause.selected, PAUSE_OPTION_ABANDON);
    ASSERT_EQ_INT(press(game_port, session, TEST_SAVE_KEY, TEST_PAUSED_AT),
                  GAME_SESSION_SAVE);
    ASSERT_EQ_INT(session->pause.selected, PAUSE_OPTION_ABANDON);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_MENU);
    ASSERT_EQ_INT(
        press(game_port, session, TEST_SAVE_KEY_UPPER, TEST_PAUSED_AT),
        GAME_SESSION_SAVE);
}

static void test_handle_game_session_key_save_request_keeps_the_selection(void)
{
    run_save_session_test(DIFFICULTY_EASY,
                          check_save_request_keeps_the_menu_selection);
}

static void check_s_while_playing_changes_nothing(const GamePort* game_port,
                                                  GameSession* session)
{
    make_edit(game_port, session);
    Board board_before = session->board;
    GameTimer timer    = session->timer;
    PauseState pause   = session->pause;
    ASSERT_EQ_INT(press(game_port, session, TEST_SAVE_KEY, TEST_START_TIME + 1),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(
        press(game_port, session, TEST_SAVE_KEY_UPPER, TEST_START_TIME + 1),
        GAME_SESSION_CONTINUE);
    ASSERT_TRUE(boards_match(&session->board, &board_before));
    ASSERT_TRUE(timers_match(&session->timer, &timer));
    ASSERT_TRUE(pause_states_match(&session->pause, &pause));
    ASSERT_FALSE(session->undo_notice);
}

static void test_handle_game_session_key_s_while_playing_changes_nothing(void)
{
    run_save_session_test(DIFFICULTY_EASY,
                          check_s_while_playing_changes_nothing);
}

static void check_s_in_a_prompt_is_ignored(const GamePort* game_port,
                                           GameSession* session)
{
    make_edit(game_port, session);
    press(game_port, session, 'N', TEST_START_TIME + 2);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PROMPT);
    PauseState pause = session->pause;
    ASSERT_EQ_INT(press(game_port, session, TEST_SAVE_KEY, TEST_START_TIME + 3),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(
        press(game_port, session, TEST_SAVE_KEY_UPPER, TEST_START_TIME + 3),
        GAME_SESSION_CONTINUE);
    ASSERT_TRUE(pause_states_match(&session->pause, &pause));
    ASSERT_EQ_INT(fake_generate_calls, 1);
}

static void test_handle_game_session_key_s_in_a_prompt_is_ignored(void)
{
    run_save_session_test(DIFFICULTY_EASY, check_s_in_a_prompt_is_ignored);
}

static void
check_save_request_with_progress_opens_no_prompt(const GamePort* game_port,
                                                 GameSession* session)
{
    make_edit(game_port, session);
    ASSERT_TRUE(session->board.history.count > 0);
    press(game_port, session, 'p', TEST_PAUSED_AT);
    ASSERT_EQ_INT(press(game_port, session, TEST_SAVE_KEY, TEST_PAUSED_AT),
                  GAME_SESSION_SAVE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_MENU);
    ASSERT_EQ_INT(session->pause.pending, GAME_ACTION_NONE);
    ASSERT_EQ_INT(fake_generate_calls, 1);
}

static void
test_handle_game_session_key_save_request_with_progress_no_prompt(void)
{
    run_save_session_test(DIFFICULTY_EASY,
                          check_save_request_with_progress_opens_no_prompt);
}

static void
check_resume_after_a_save_request_plays_on(const GamePort* game_port,
                                           GameSession* session)
{
    press(game_port, session, 'p', TEST_PAUSED_AT);
    press(game_port, session, TEST_SAVE_KEY, TEST_PAUSED_AT);
    ASSERT_EQ_INT(press(game_port, session, TEST_ESCAPE, TEST_PAUSED_AT + 50),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_FALSE(session->timer.paused);
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 30);
    make_edit(game_port, session);
    ASSERT_EQ_INT(game_port->board_move_count_f(&session->board), 1);
}

static void test_handle_game_session_key_resume_after_a_save_request(void)
{
    run_save_session_test(DIFFICULTY_EASY,
                          check_resume_after_a_save_request_plays_on);
}

static void test_init_game_session_is_not_linked_to_a_save(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    GameSession session;
    memset(&session, TEST_GARBAGE_BYTE, sizeof session);
    bool started = start_session(game_port, &session, TEST_START_TIME);
    bool linked  = session.linked_to_save;
    end_game_session(game_port, &session);
    destroy_game_port(game_port);
    ASSERT_TRUE(started);
    ASSERT_FALSE(linked);
}

static void test_init_game_session_from_save_restores_the_game(void)
{
    GamePort* game_port = create_save_port(DIFFICULTY_EASY);
    ASSERT_NOT_NULL(game_port);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    GameSession session;
    memset(&session, TEST_GARBAGE_BYTE, sizeof session);
    bool started =
        init_game_session_from_save(game_port, &session, &saved, &board_codes,
                                    &menu_codes, TEST_CONTINUED_AT);
    int generated = fake_generate_calls;
    ASSERT_TRUE(started);
    bool owned   = session.puzzle != NULL && session.puzzle != &saved.puzzle &&
                   session.board.puzzle == session.puzzle &&
                   number_generations_equal(session.puzzle, &saved.puzzle);
    bool entries = true;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        entries = entries && session.board.slots[i].editable ==
                                 (saved.puzzle.available_array[i] ==
                                  TETONOR_HIDDEN_NUMBER);
        entries = entries &&
                  session.board.slots[i].value == saved.entries.slot_values[i];
        entries = entries && operand_groups_equal(&session.board.groups[i],
                                                  &saved.entries.groups[i]);
    }
    bool timer_ok = session.timer.elapsed_seconds == SAVE_FIXTURE_ELAPSED &&
                    session.timer.running && !session.timer.paused &&
                    session.timer.last_tick == TEST_CONTINUED_AT;
    bool state_ok = session.pause.screen == PAUSE_SCREEN_PLAYING &&
                    !is_victory_shown(&session.victory) &&
                    !session.undo_notice && session.linked_to_save;
    bool keys_ok  = session.board_key_codes.enter == TEST_ENTER_KEY &&
                    session.board_key_codes.pad_star == TEST_PAD_STAR_KEY &&
                    session.menu_key_codes.up == TEST_UP_KEY &&
                    session.menu_key_codes.down == TEST_DOWN_KEY;
    int moves     = game_port->board_move_count_f(&session.board);
    int history   = session.board.history.count;
    BoardResult result = session.board.result;
    int cursor         = session.board.cursor;
    end_game_session(game_port, &session);
    int freed = fake_free_calls;
    destroy_game_port(game_port);
    ASSERT_EQ_INT(generated, 0);
    ASSERT_TRUE(owned);
    ASSERT_TRUE(entries);
    ASSERT_EQ_INT(moves, SAVE_FIXTURE_MOVES);
    ASSERT_EQ_INT(history, 0);
    ASSERT_EQ_INT(result, BOARD_RESULT_PENDING);
    ASSERT_EQ_INT(cursor, TEST_RESTORED_CURSOR);
    ASSERT_TRUE(timer_ok);
    ASSERT_TRUE(state_ok);
    ASSERT_TRUE(keys_ok);
    ASSERT_EQ_INT(freed, 1);
}

static void test_init_game_session_from_save_invalid_save_owns_nothing(void)
{
    GamePort* game_port = create_save_port(DIFFICULTY_EASY);
    ASSERT_NOT_NULL(game_port);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    saved.elapsed_seconds = -1;
    GameSession session;
    bool started =
        init_game_session_from_save(game_port, &session, &saved, &board_codes,
                                    &menu_codes, TEST_START_TIME);
    int generated = fake_generate_calls;
    int freed     = fake_free_calls;
    destroy_game_port(game_port);
    ASSERT_FALSE(started);
    ASSERT_EQ_INT(generated, 0);
    ASSERT_EQ_INT(freed, 0);
}

static void
test_init_game_session_from_save_ignores_the_time_since_the_save(void)
{
    GamePort* game_port = create_save_port(DIFFICULTY_EASY);
    ASSERT_NOT_NULL(game_port);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    GameSession session;
    ASSERT_TRUE(init_game_session_from_save(game_port, &session, &saved,
                                            &board_codes, &menu_codes,
                                            TEST_CONTINUED_AT));
    press(game_port, &session, 'p', TEST_CONTINUED_AT + 5);
    int32_t seconds = session.timer.elapsed_seconds;
    end_game_session(game_port, &session);
    destroy_game_port(game_port);
    ASSERT_EQ_INT(seconds, SAVE_FIXTURE_ELAPSED + 5);
}

static void
check_continued_game_new_game_key_starts_at_once(const GamePort* game_port,
                                                 GameSession* session)
{
    ASSERT_TRUE(session->linked_to_save);
    ASSERT_FALSE(game_port->board_progress_would_be_lost_f(&session->board));
    ASSERT_EQ_INT(press(game_port, session, 'N', TEST_CONTINUED_AT),
                  GAME_SESSION_RESTARTED);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_FALSE(session->linked_to_save);
    ASSERT_EQ_INT(fake_generate_calls, 1);
    ASSERT_EQ_INT(fake_free_calls, 1);
    ASSERT_TRUE(session_is_fresh(session, TEST_CONTINUED_AT));
}

static void test_restart_new_game_unlinks_a_continued_game(void)
{
    run_continued_session_test(
        check_continued_game_new_game_key_starts_at_once);
}

static void
check_continued_game_restart_button_unlinks(const GamePort* game_port,
                                            GameSession* session)
{
    session->board.focus = BOARD_FOCUS_RESTART;
    ASSERT_EQ_INT(press(game_port, session, TEST_ENTER_KEY, TEST_CONTINUED_AT),
                  GAME_SESSION_RESTARTED);
    ASSERT_FALSE(session->linked_to_save);
}

static void test_restart_button_unlinks_a_continued_game(void)
{
    run_continued_session_test(check_continued_game_restart_button_unlinks);
}

static void check_confirmed_new_game_unlinks(const GamePort* game_port,
                                             GameSession* session)
{
    make_edit(game_port, session);
    ASSERT_EQ_INT(press(game_port, session, 'N', TEST_CONTINUED_AT),
                  GAME_SESSION_CONTINUE);
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PROMPT);
    ASSERT_TRUE(session->linked_to_save);
    ASSERT_EQ_INT(press(game_port, session, 'y', TEST_CONTINUED_AT),
                  GAME_SESSION_RESTARTED);
    ASSERT_FALSE(session->linked_to_save);
}

static void test_confirmed_new_game_unlinks_a_continued_game(void)
{
    run_continued_session_test(check_confirmed_new_game_unlinks);
}

static void check_cancelled_new_game_keeps_the_link(const GamePort* game_port,
                                                    GameSession* session)
{
    make_edit(game_port, session);
    press(game_port, session, 'N', TEST_CONTINUED_AT);
    ASSERT_EQ_INT(press(game_port, session, TETONOR_KEY_NO, TEST_CONTINUED_AT),
                  GAME_SESSION_CONTINUE);
    ASSERT_TRUE(session->linked_to_save);
    ASSERT_EQ_INT(fake_generate_calls, 0);
}

static void test_cancelled_new_game_keeps_a_continued_game_linked(void)
{
    run_continued_session_test(check_cancelled_new_game_keeps_the_link);
}

static void check_failed_new_game_keeps_the_link(const GamePort* game_port,
                                                 GameSession* session)
{
    fake_generate_fails = true;
    ASSERT_EQ_INT(press(game_port, session, 'N', TEST_CONTINUED_AT),
                  GAME_SESSION_CONTINUE);
    ASSERT_TRUE(session->linked_to_save);
    ASSERT_EQ_INT(game_port->board_move_count_f(&session->board),
                  SAVE_FIXTURE_MOVES);
}

static void test_failed_new_game_keeps_a_continued_game_linked(void)
{
    run_continued_session_test(check_failed_new_game_keeps_the_link);
}

static void
check_leaving_keeps_the_link_of_the_ended_game(const GamePort* game_port,
                                               GameSession* session)
{
    ASSERT_EQ_INT(press(game_port, session, TEST_ESCAPE, TEST_CONTINUED_AT),
                  GAME_SESSION_LEAVE);
    ASSERT_TRUE(session->linked_to_save);
}

static void test_leaving_a_continued_game_does_not_unlink_it(void)
{
    run_continued_session_test(check_leaving_keeps_the_link_of_the_ended_game);
}

static void
check_capture_uses_the_timer_and_the_port_difficulty(const GamePort* game_port,
                                                     GameSession* session)
{
    make_edit(game_port, session);
    press(game_port, session, 'p', TEST_PAUSED_AT);
    SavedGame saved;
    memset(&saved, TEST_GARBAGE_BYTE, sizeof saved);
    ASSERT_TRUE(capture_game_session(game_port, session, &saved));
    ASSERT_EQ_INT(saved.difficulty, game_port->difficulty);
    ASSERT_EQ_INT(saved.elapsed_seconds, 30);
    ASSERT_EQ_INT(saved.move_count, 1);
    ASSERT_EQ_INT(saved.entries.slot_values[session->board.cursor],
                  TEST_TYPED_DIGIT - '0');
    ASSERT_EQ_INT(saved.entries.result, BOARD_RESULT_PENDING);
    ASSERT_TRUE(number_generations_equal(&saved.puzzle, session->puzzle));
    ASSERT_TRUE(is_saved_game_valid(&saved));
}

static void test_capture_game_session_reads_the_timer_and_the_port(void)
{
    run_save_session_test(DIFFICULTY_EASY,
                          check_capture_uses_the_timer_and_the_port_difficulty);
    run_save_session_test(DIFFICULTY_MEDIUM,
                          check_capture_uses_the_timer_and_the_port_difficulty);
    run_save_session_test(DIFFICULTY_HARD,
                          check_capture_uses_the_timer_and_the_port_difficulty);
}

static void check_capture_of_a_solved_game_fails(const GamePort* game_port,
                                                 GameSession* session)
{
    session->board.result = BOARD_RESULT_CORRECT;
    SavedGame saved;
    memset(&saved, TEST_GARBAGE_BYTE, sizeof saved);
    unsigned char before[sizeof saved];
    memcpy(before, &saved, sizeof saved);
    ASSERT_FALSE(capture_game_session(game_port, session, &saved));
    ASSERT_EQ_INT(memcmp(before, &saved, sizeof saved), 0);
}

static void test_capture_game_session_solved_game_fails_untouched(void)
{
    run_save_session_test(DIFFICULTY_EASY,
                          check_capture_of_a_solved_game_fails);
}

static void
check_continue_then_capture_gives_the_same_save(const GamePort* game_port,
                                                GameSession* session)
{
    press(game_port, session, 'p', TEST_START_TIME);
    SavedGame expected;
    build_saved_game_fixture(&expected);
    SavedGame captured;
    ASSERT_TRUE(capture_game_session(game_port, session, &captured));
    ASSERT_TRUE(saved_games_equal(&captured, &expected));
}

static void test_continue_then_capture_returns_the_same_save(void)
{
    run_continued_session_test(check_continue_then_capture_gives_the_same_save);
}

static void check_note_saved_links_the_session(const GamePort* game_port,
                                               GameSession* session)
{
    ASSERT_FALSE(session->linked_to_save);
    press(game_port, session, 'p', TEST_PAUSED_AT);
    note_game_session_saved(session, PAUSE_SAVE_NOTICE_SAVED);
    ASSERT_EQ_INT(session->pause.save_notice, PAUSE_SAVE_NOTICE_SAVED);
    ASSERT_TRUE(session->linked_to_save);
}

static void test_note_game_session_saved_links_the_session(void)
{
    run_save_session_test(DIFFICULTY_EASY, check_note_saved_links_the_session);
}

static void check_note_session_only_links_the_session(const GamePort* game_port,
                                                      GameSession* session)
{
    press(game_port, session, 'p', TEST_PAUSED_AT);
    note_game_session_saved(session, PAUSE_SAVE_NOTICE_SESSION_ONLY);
    ASSERT_EQ_INT(session->pause.save_notice, PAUSE_SAVE_NOTICE_SESSION_ONLY);
    ASSERT_TRUE(session->linked_to_save);
}

static void test_note_game_session_saved_session_only_links_the_session(void)
{
    run_save_session_test(DIFFICULTY_EASY,
                          check_note_session_only_links_the_session);
}

static void check_note_failed_does_not_link(const GamePort* game_port,
                                            GameSession* session)
{
    press(game_port, session, 'p', TEST_PAUSED_AT);
    note_game_session_saved(session, PAUSE_SAVE_NOTICE_FAILED);
    ASSERT_EQ_INT(session->pause.save_notice, PAUSE_SAVE_NOTICE_FAILED);
    ASSERT_FALSE(session->linked_to_save);
}

static void test_note_game_session_saved_failed_does_not_link(void)
{
    run_save_session_test(DIFFICULTY_EASY, check_note_failed_does_not_link);
}

static void check_note_failed_keeps_an_existing_link(const GamePort* game_port,
                                                     GameSession* session)
{
    press(game_port, session, 'p', TEST_PAUSED_AT);
    note_game_session_saved(session, PAUSE_SAVE_NOTICE_SAVED);
    note_game_session_saved(session, PAUSE_SAVE_NOTICE_FAILED);
    ASSERT_EQ_INT(session->pause.save_notice, PAUSE_SAVE_NOTICE_FAILED);
    ASSERT_TRUE(session->linked_to_save);
}

static void test_note_game_session_saved_failed_keeps_an_existing_link(void)
{
    run_save_session_test(DIFFICULTY_EASY,
                          check_note_failed_keeps_an_existing_link);
}

static void check_notice_lives_while_the_menu_is_open(const GamePort* game_port,
                                                      GameSession* session)
{
    press(game_port, session, 'p', TEST_PAUSED_AT);
    note_game_session_saved(session, PAUSE_SAVE_NOTICE_SAVED);
    press(game_port, session, TEST_DOWN_KEY, TEST_PAUSED_AT);
    press(game_port, session, TEST_UP_KEY, TEST_PAUSED_AT);
    ASSERT_EQ_INT(session->pause.save_notice, PAUSE_SAVE_NOTICE_SAVED);
    press(game_port, session, TEST_ESCAPE, TEST_PAUSED_AT);
    press(game_port, session, 'p', TEST_PAUSED_AT);
    ASSERT_EQ_INT(session->pause.save_notice, PAUSE_SAVE_NOTICE_NONE);
}

static void test_save_notice_lasts_until_the_pause_menu_closes(void)
{
    run_save_session_test(DIFFICULTY_EASY,
                          check_notice_lives_while_the_menu_is_open);
}

static void check_continued_game_completes_once_with_the_summed_summary(
    const GamePort* game_port, GameSession* session)
{
    memcpy(session->board.groups, session->puzzle->solution_groups,
           sizeof session->board.groups);
    const int keys[] = {
        TEST_RIGHT_KEY, TEST_SECOND_HIDDEN_SLOT_VALUE, TEST_RIGHT_KEY,
        TEST_RIGHT_KEY, TEST_LAST_HIDDEN_SLOT_VALUE,   TEST_RIGHT_KEY,
        TEST_ENTER_KEY};
    press_all(game_port, session, keys, sizeof keys / sizeof keys[0],
              TEST_CONTINUED_AT);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(update_game_session_timer(game_port, session,
                                          TEST_CONTINUED_COMPLETED_AT));
    ASSERT_TRUE(is_victory_shown(&session->victory));
    ASSERT_FALSE(update_game_session_timer(game_port, session,
                                           TEST_CONTINUED_COMPLETED_AT + 5));
    VictorySummary expected;
    build_victory_summary(
        SAVE_FIXTURE_ELAPSED + TEST_CONTINUED_EXTRA_SECONDS, DIFFICULTY_EASY,
        SAVE_FIXTURE_MOVES + TEST_CONTINUED_EXTRA_MOVES, &expected);
    for (int line = 0; line < VICTORY_SUMMARY_LINE_COUNT; line++)
    {
        ASSERT_EQ_INT(
            strcmp(session->victory.summary.lines[line], expected.lines[line]),
            0);
    }
}

static void
test_a_continued_game_completes_once_with_restored_moves_and_time(void)
{
    run_continued_session_test(
        check_continued_game_completes_once_with_the_summed_summary);
}

/* A slot above the given 3 after it, then (1 + 9) in a cell whose target is
 * 3: two conflicts, none of which may touch the timer, the pause state or the
 * verdict. */
static void
check_conflicting_entries_keep_timer_and_pause(const GamePort* game_port,
                                               GameSession* session)
{
    const int keys[] = {TEST_TYPED_DIGIT, TEST_UP_KEY, '1', TEST_RIGHT_KEY, '+',
                        TEST_RIGHT_KEY,   '9'};
    GameTimer timer_before = session->timer;
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        ASSERT_EQ_INT(press(game_port, session, keys[i], TEST_START_TIME),
                      GAME_SESSION_CONTINUE);
    }
    ASSERT_TRUE(session->board.conflicts.slots[TEST_FIRST_SLOT]);
    ASSERT_TRUE(group_has_conflict(&session->board.conflicts, 0));
    ASSERT_EQ_INT(session->board.conflicts.count, 2);
    ASSERT_TRUE(timers_match(&timer_before, &session->timer));
    ASSERT_EQ_INT(session->pause.screen, PAUSE_SCREEN_PLAYING);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_PENDING);
    ASSERT_FALSE(session->undo_notice);
}

static void
test_handle_game_session_key_conflicting_entry_keeps_timer_and_pause(void)
{
    run_session_test(check_conflicting_entries_keep_timer_and_pause);
}

#define TEST_HINT_KEY 'h'
#define TEST_HINT_KEY_UPPER 'H'
#define TEST_HINT_AT (TEST_START_TIME + 5)
#define TEST_AFTER_HINT_AT (TEST_START_TIME + 9)
#define TEST_CONFLICTING_DIGIT '9'

static void check_h_shows_a_hint_for_the_focused_slot(const GamePort* game_port,
                                                      GameSession* session)
{
    ASSERT_FALSE(session->hint_shown);
    ASSERT_EQ_INT(press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT),
                  GAME_SESSION_CONTINUE);
    ASSERT_TRUE(session->hint_shown);
    ASSERT_EQ_INT(session->hint.status, HINT_STATUS_SUGGESTION);
    ASSERT_EQ_INT(session->hint.target, HINT_TARGET_SLOT);
    ASSERT_EQ_INT(session->hint.index, TEST_FIRST_SLOT);
    ASSERT_EQ_INT(session->hint.value, fixture_strip[TEST_FIRST_SLOT]);
}

static void test_handle_game_session_key_h_shows_a_hint(void)
{
    run_session_test(check_h_shows_a_hint_for_the_focused_slot);
}

static void check_upper_h_shows_a_hint(const GamePort* game_port,
                                       GameSession* session)
{
    press(game_port, session, TEST_HINT_KEY_UPPER, TEST_HINT_AT);
    ASSERT_TRUE(session->hint_shown);
    ASSERT_EQ_INT(session->hint.status, HINT_STATUS_SUGGESTION);
}

static void test_handle_game_session_key_upper_h_shows_a_hint(void)
{
    run_session_test(check_upper_h_shows_a_hint);
}

static void
check_hint_changes_no_board_timer_or_pause_state(const GamePort* game_port,
                                                 GameSession* session)
{
    press(game_port, session, '2', TEST_START_TIME);
    press(game_port, session, TEST_RIGHT_KEY, TEST_START_TIME);
    GameSession before = *session;
    int moves          = game_port->board_move_count_f(&session->board);
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    ASSERT_TRUE(session->hint_shown);
    ASSERT_TRUE(boards_match(&session->board, &before.board));
    ASSERT_EQ_INT(game_port->board_move_count_f(&session->board), moves);
    ASSERT_EQ_INT(session->board.history.count, before.board.history.count);
    ASSERT_EQ_INT(session->board.result, before.board.result);
    ASSERT_TRUE(timers_match(&session->timer, &before.timer));
    ASSERT_TRUE(pause_states_match(&session->pause, &before.pause));
    ASSERT_FALSE(is_game_paused(&session->pause));
    ASSERT_FALSE(session->undo_notice);
    ASSERT_FALSE(is_victory_shown(&session->victory));
}

static void test_handle_game_session_key_hint_changes_nothing_but_the_hint(void)
{
    run_session_test(check_hint_changes_no_board_timer_or_pause_state);
}

static void check_a_move_clears_the_hint(const GamePort* game_port,
                                         GameSession* session)
{
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    ASSERT_TRUE(session->hint_shown);
    make_edit(game_port, session);
    ASSERT_FALSE(session->hint_shown);
}

static void test_handle_game_session_key_a_move_clears_the_hint(void)
{
    run_session_test(check_a_move_clears_the_hint);
}

static void check_a_group_edit_clears_the_hint(const GamePort* game_port,
                                               GameSession* session)
{
    session->board.focus        = BOARD_FOCUS_GROUP;
    session->board.group_cursor = 0;
    session->board.group_field  = GROUP_FIELD_OPERATION;
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    ASSERT_TRUE(session->hint_shown);
    press(game_port, session, '+', TEST_AFTER_HINT_AT);
    ASSERT_EQ_INT(game_port->board_move_count_f(&session->board), 1);
    ASSERT_FALSE(session->hint_shown);
}

static void test_handle_game_session_key_a_group_edit_clears_the_hint(void)
{
    run_session_test(check_a_group_edit_clears_the_hint);
}

static void check_a_cursor_move_keeps_the_hint(const GamePort* game_port,
                                               GameSession* session)
{
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    const int keys[] = {TEST_RIGHT_KEY, TEST_LEFT_KEY, TEST_DOWN_KEY,
                        TEST_UP_KEY};
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    {
        press(game_port, session, keys[i], TEST_AFTER_HINT_AT);
        ASSERT_TRUE(session->hint_shown);
        ASSERT_EQ_INT(session->hint.status, HINT_STATUS_SUGGESTION);
    }
    ASSERT_EQ_INT(game_port->board_move_count_f(&session->board), 0);
}

static void test_handle_game_session_key_a_cursor_move_keeps_the_hint(void)
{
    run_session_test(check_a_cursor_move_keeps_the_hint);
}

static void check_an_ignored_key_keeps_the_hint(const GamePort* game_port,
                                                GameSession* session)
{
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    press(game_port, session, 'q', TEST_AFTER_HINT_AT);
    press(game_port, session, TEST_RESIZE_KEY, TEST_AFTER_HINT_AT);
    press(game_port, session, TEST_ERR_KEY, TEST_AFTER_HINT_AT);
    ASSERT_TRUE(session->hint_shown);
}

static void test_handle_game_session_key_an_ignored_key_keeps_the_hint(void)
{
    run_session_test(check_an_ignored_key_keeps_the_hint);
}

static void check_a_successful_undo_clears_the_hint(const GamePort* game_port,
                                                    GameSession* session)
{
    make_edit(game_port, session);
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    ASSERT_TRUE(session->hint_shown);
    press(game_port, session, 'u', TEST_AFTER_HINT_AT);
    ASSERT_FALSE(session->undo_notice);
    ASSERT_FALSE(session->hint_shown);
}

static void test_handle_game_session_key_a_successful_undo_clears_the_hint(void)
{
    run_session_test(check_a_successful_undo_clears_the_hint);
}

static void check_an_empty_undo_keeps_the_hint(const GamePort* game_port,
                                               GameSession* session)
{
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    press(game_port, session, 'u', TEST_AFTER_HINT_AT);
    ASSERT_TRUE(session->undo_notice);
    ASSERT_TRUE(session->hint_shown);
}

static void test_handle_game_session_key_an_empty_undo_keeps_the_hint(void)
{
    run_session_test(check_an_empty_undo_keeps_the_hint);
}

static void check_a_new_hint_replaces_the_old_one(const GamePort* game_port,
                                                  GameSession* session)
{
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    ASSERT_EQ_INT(session->hint.index, TEST_FIRST_SLOT);
    press(game_port, session, TEST_RIGHT_KEY, TEST_AFTER_HINT_AT);
    press(game_port, session, TEST_HINT_KEY, TEST_AFTER_HINT_AT);
    ASSERT_TRUE(session->hint_shown);
    ASSERT_EQ_INT(session->hint.status, HINT_STATUS_SUGGESTION);
    ASSERT_EQ_INT(session->hint.target, HINT_TARGET_SLOT);
    ASSERT_EQ_INT(session->hint.index, TEST_SECOND_SLOT);
    ASSERT_EQ_INT(session->hint.value, fixture_strip[TEST_SECOND_SLOT]);
}

static void test_handle_game_session_key_a_new_hint_replaces_the_old_one(void)
{
    run_session_test(check_a_new_hint_replaces_the_old_one);
}

static void
check_a_hint_never_pauses_or_freezes_the_timer(const GamePort* game_port,
                                               GameSession* session)
{
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    ASSERT_FALSE(session->timer.paused);
    ASSERT_TRUE(session->timer.running);
    ASSERT_FALSE(
        update_game_session_timer(game_port, session, TEST_START_TIME + 12));
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 12);
}

static void test_handle_game_session_key_a_hint_does_not_pause_the_timer(void)
{
    run_session_test(check_a_hint_never_pauses_or_freezes_the_timer);
}

static void check_h_is_ignored_in_the_pause_menu(const GamePort* game_port,
                                                 GameSession* session)
{
    press(game_port, session, 'p', TEST_HINT_AT);
    ASSERT_TRUE(is_game_paused(&session->pause));
    press(game_port, session, TEST_HINT_KEY, TEST_AFTER_HINT_AT);
    press(game_port, session, TEST_HINT_KEY_UPPER, TEST_AFTER_HINT_AT);
    ASSERT_FALSE(session->hint_shown);
    ASSERT_TRUE(game_session_counts_for_statistics(game_port, session));
}

static void test_handle_game_session_key_h_is_ignored_in_the_pause_menu(void)
{
    run_session_test(check_h_is_ignored_in_the_pause_menu);
}

static void check_h_is_ignored_on_the_theme_screen(const GamePort* game_port,
                                                   GameSession* session)
{
    press(game_port, session, 't', TEST_HINT_AT);
    ASSERT_TRUE(is_theme_screen_shown(&session->pause));
    press(game_port, session, TEST_HINT_KEY, TEST_AFTER_HINT_AT);
    ASSERT_FALSE(session->hint_shown);
    ASSERT_TRUE(game_session_counts_for_statistics(game_port, session));
}

static void test_handle_game_session_key_h_is_ignored_on_the_theme_screen(void)
{
    run_session_test(check_h_is_ignored_on_the_theme_screen);
}

static void check_h_is_ignored_on_the_victory_screen(const GamePort* game_port,
                                                     GameSession* session)
{
    complete_session(game_port, session);
    press(game_port, session, TEST_HINT_KEY, TEST_COMPLETED_AT);
    ASSERT_FALSE(session->hint_shown);
    ASSERT_TRUE(is_victory_shown(&session->victory));
    ASSERT_TRUE(game_session_counts_for_statistics(game_port, session));
}

static void
test_handle_game_session_key_h_is_ignored_on_the_victory_screen(void)
{
    run_session_test(check_h_is_ignored_on_the_victory_screen);
}

static void
check_counts_for_statistics_before_and_after_a_hint(const GamePort* game_port,
                                                    GameSession* session)
{
    ASSERT_TRUE(game_session_counts_for_statistics(game_port, session));
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    ASSERT_EQ_INT(session->hint.status, HINT_STATUS_SUGGESTION);
    ASSERT_FALSE(game_session_counts_for_statistics(game_port, session));
}

static void test_game_session_counts_for_statistics_false_after_a_hint(void)
{
    run_session_test(check_counts_for_statistics_before_and_after_a_hint);
}

static void
check_the_mark_outlives_a_move_and_an_undo(const GamePort* game_port,
                                           GameSession* session)
{
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    make_edit(game_port, session);
    ASSERT_FALSE(session->hint_shown);
    ASSERT_FALSE(game_session_counts_for_statistics(game_port, session));
    press(game_port, session, 'u', TEST_AFTER_HINT_AT);
    ASSERT_FALSE(session->hint_shown);
    ASSERT_FALSE(game_session_counts_for_statistics(game_port, session));
}

static void
test_game_session_counts_for_statistics_stays_false_after_moves(void)
{
    run_session_test(check_the_mark_outlives_a_move_and_an_undo);
}

static void check_a_conflict_reply_does_not_mark(const GamePort* game_port,
                                                 GameSession* session)
{
    press(game_port, session, TEST_CONFLICTING_DIGIT, TEST_START_TIME);
    ASSERT_TRUE(session->board.conflicts.count > 0);
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    ASSERT_TRUE(session->hint_shown);
    ASSERT_EQ_INT(session->hint.status, HINT_STATUS_CONFLICTS);
    ASSERT_TRUE(game_session_counts_for_statistics(game_port, session));
}

static void test_game_session_conflict_reply_does_not_mark_the_game(void)
{
    run_session_test(check_a_conflict_reply_does_not_mark);
}

static void check_a_full_board_reply_does_not_mark(const GamePort* game_port,
                                                   GameSession* session)
{
    fill_without_confirming(game_port, session, TEST_START_TIME);
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    ASSERT_TRUE(session->hint_shown);
    ASSERT_EQ_INT(session->hint.status, HINT_STATUS_BOARD_FULL);
    ASSERT_TRUE(game_session_counts_for_statistics(game_port, session));
}

static void test_game_session_full_board_reply_does_not_mark_the_game(void)
{
    run_session_test(check_a_full_board_reply_does_not_mark);
}

static void check_a_solved_board_reply_does_not_mark(const GamePort* game_port,
                                                     GameSession* session)
{
    solve_session(game_port, session, TEST_SOLVED_AT);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
    press(game_port, session, TEST_HINT_KEY, TEST_SOLVED_AT);
    ASSERT_TRUE(session->hint_shown);
    ASSERT_EQ_INT(session->hint.status, HINT_STATUS_SOLVED);
    ASSERT_TRUE(game_session_counts_for_statistics(game_port, session));
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
}

static void test_game_session_solved_board_reply_does_not_mark_the_game(void)
{
    run_session_test(check_a_solved_board_reply_does_not_mark);
}

static void
check_a_hinted_game_completes_but_does_not_count(const GamePort* game_port,
                                                 GameSession* session)
{
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    ASSERT_EQ_INT(session->hint.status, HINT_STATUS_SUGGESTION);
    solve_session(game_port, session, TEST_SOLVED_AT);
    ASSERT_EQ_INT(session->board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(
        update_game_session_timer(game_port, session, TEST_COMPLETED_AT));
    ASSERT_TRUE(is_victory_shown(&session->victory));
    ASSERT_EQ_INT(session->timer.elapsed_seconds, 10);
    ASSERT_FALSE(game_session_counts_for_statistics(game_port, session));
    ASSERT_FALSE(
        update_game_session_timer(game_port, session, TEST_COMPLETED_AT + 1));
}

static void test_a_hinted_game_completes_but_does_not_count(void)
{
    run_session_test(check_a_hinted_game_completes_but_does_not_count);
}

static void
check_an_unhinted_game_completes_and_counts(const GamePort* game_port,
                                            GameSession* session)
{
    solve_session(game_port, session, TEST_SOLVED_AT);
    ASSERT_TRUE(
        update_game_session_timer(game_port, session, TEST_COMPLETED_AT));
    ASSERT_TRUE(is_victory_shown(&session->victory));
    ASSERT_TRUE(game_session_counts_for_statistics(game_port, session));
}

static void test_an_unhinted_game_completes_and_counts(void)
{
    run_session_test(check_an_unhinted_game_completes_and_counts);
}

static void check_restart_by_key_clears_the_hint(const GamePort* game_port,
                                                 GameSession* session)
{
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    ASSERT_FALSE(game_session_counts_for_statistics(game_port, session));
    ASSERT_EQ_INT(press(game_port, session, 'n', TEST_RESTART_AT),
                  GAME_SESSION_RESTARTED);
    ASSERT_FALSE(session->hint_shown);
    ASSERT_TRUE(game_session_counts_for_statistics(game_port, session));
}

static void test_a_new_game_by_key_clears_the_hint_and_the_mark(void)
{
    run_session_test(check_restart_by_key_clears_the_hint);
}

static void check_restart_by_button_clears_the_hint(const GamePort* game_port,
                                                    GameSession* session)
{
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    session->board.focus = BOARD_FOCUS_RESTART;
    ASSERT_EQ_INT(press(game_port, session, TEST_ENTER_KEY, TEST_RESTART_AT),
                  GAME_SESSION_RESTARTED);
    ASSERT_FALSE(session->hint_shown);
    ASSERT_TRUE(game_session_counts_for_statistics(game_port, session));
}

static void test_the_restart_button_clears_the_hint_and_the_mark(void)
{
    run_session_test(check_restart_by_button_clears_the_hint);
}

static void
check_a_confirmed_new_game_clears_the_hint(const GamePort* game_port,
                                           GameSession* session)
{
    press(game_port, session, '2', TEST_START_TIME);
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    ASSERT_EQ_INT(session->hint.status, HINT_STATUS_SUGGESTION);
    press(game_port, session, 'n', TEST_AFTER_HINT_AT);
    ASSERT_TRUE(session->hint_shown);
    ASSERT_FALSE(game_session_counts_for_statistics(game_port, session));
    ASSERT_EQ_INT(press(game_port, session, 'y', TEST_AFTER_HINT_AT),
                  GAME_SESSION_RESTARTED);
    ASSERT_FALSE(session->hint_shown);
    ASSERT_TRUE(game_session_counts_for_statistics(game_port, session));
}

static void test_a_confirmed_new_game_clears_the_hint_and_the_mark(void)
{
    run_session_test(check_a_confirmed_new_game_clears_the_hint);
}

static void check_a_failed_restart_keeps_the_hint(const GamePort* game_port,
                                                  GameSession* session)
{
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    fake_generate_fails = true;
    ASSERT_EQ_INT(press(game_port, session, 'n', TEST_RESTART_AT),
                  GAME_SESSION_CONTINUE);
    ASSERT_TRUE(session->hint_shown);
    ASSERT_FALSE(game_session_counts_for_statistics(game_port, session));
}

static void test_a_failed_new_game_keeps_the_hint_and_the_mark(void)
{
    run_session_test(check_a_failed_restart_keeps_the_hint);
}

static void check_a_new_game_gets_its_own_hint(const GamePort* game_port,
                                               GameSession* session)
{
    press(game_port, session, TEST_HINT_KEY, TEST_HINT_AT);
    press(game_port, session, 'n', TEST_RESTART_AT);
    ASSERT_FALSE(session->hint_shown);
    press(game_port, session, TEST_HINT_KEY, TEST_RESTART_AT + 1);
    ASSERT_TRUE(session->hint_shown);
    ASSERT_EQ_INT(session->hint.status, HINT_STATUS_SUGGESTION);
    ASSERT_FALSE(game_session_counts_for_statistics(game_port, session));
}

static void test_a_new_game_can_be_hinted_again(void)
{
    run_session_test(check_a_new_game_gets_its_own_hint);
}

static void test_init_game_session_shows_no_hint(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    GameSession session;
    memset(&session, TEST_GARBAGE_BYTE, sizeof session);
    bool started = start_session(game_port, &session, TEST_START_TIME);
    bool shown   = started && session.hint_shown;
    bool counts =
        started && game_session_counts_for_statistics(game_port, &session);
    end_game_session(game_port, &session);
    destroy_game_port(game_port);
    ASSERT_TRUE(started);
    ASSERT_FALSE(shown);
    ASSERT_TRUE(counts);
}

static void test_init_game_session_from_save_shows_no_hint(void)
{
    GamePort* game_port = create_save_port(DIFFICULTY_EASY);
    ASSERT_NOT_NULL(game_port);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    GameSession session;
    memset(&session, TEST_GARBAGE_BYTE, sizeof session);
    bool started =
        init_game_session_from_save(game_port, &session, &saved, &board_codes,
                                    &menu_codes, TEST_CONTINUED_AT);
    bool shown = started && session.hint_shown;
    bool counts =
        started && game_session_counts_for_statistics(game_port, &session);
    end_game_session(game_port, &session);
    destroy_game_port(game_port);
    ASSERT_TRUE(started);
    ASSERT_FALSE(shown);
    ASSERT_TRUE(counts);
}

static void test_a_game_continued_from_a_hinted_save_does_not_count(void)
{
    GamePort* game_port = create_save_port(DIFFICULTY_EASY);
    ASSERT_NOT_NULL(game_port);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    saved.hint_used = true;
    GameSession session;
    memset(&session, TEST_GARBAGE_BYTE, sizeof session);
    bool started =
        init_game_session_from_save(game_port, &session, &saved, &board_codes,
                                    &menu_codes, TEST_CONTINUED_AT);
    bool shown = started && session.hint_shown;
    bool counts =
        started && game_session_counts_for_statistics(game_port, &session);
    end_game_session(game_port, &session);
    destroy_game_port(game_port);
    ASSERT_TRUE(started);
    ASSERT_FALSE(shown);
    ASSERT_FALSE(counts);
}

static void test_the_hint_mark_survives_capturing_and_continuing(void)
{
    GamePort* game_port = create_save_port(DIFFICULTY_EASY);
    ASSERT_NOT_NULL(game_port);
    GameSession first;
    ASSERT_TRUE(start_session(game_port, &first, TEST_START_TIME));
    press(game_port, &first, TEST_HINT_KEY, TEST_HINT_AT);
    make_edit(game_port, &first);
    SavedGame saved;
    bool captured = capture_game_session(game_port, &first, &saved);
    end_game_session(game_port, &first);
    GameSession second;
    bool continued = captured && init_game_session_from_save(
                                     game_port, &second, &saved, &board_codes,
                                     &menu_codes, TEST_CONTINUED_AT);
    bool counts =
        continued && game_session_counts_for_statistics(game_port, &second);
    if (continued)
    {
        end_game_session(game_port, &second);
    }
    destroy_game_port(game_port);
    ASSERT_TRUE(captured);
    ASSERT_TRUE(saved.hint_used);
    ASSERT_TRUE(continued);
    ASSERT_FALSE(counts);
}

static void test_a_game_saved_without_a_hint_counts_when_continued(void)
{
    GamePort* game_port = create_save_port(DIFFICULTY_EASY);
    ASSERT_NOT_NULL(game_port);
    GameSession first;
    ASSERT_TRUE(start_session(game_port, &first, TEST_START_TIME));
    make_edit(game_port, &first);
    SavedGame saved;
    bool captured = capture_game_session(game_port, &first, &saved);
    end_game_session(game_port, &first);
    GameSession second;
    bool continued = captured && init_game_session_from_save(
                                     game_port, &second, &saved, &board_codes,
                                     &menu_codes, TEST_CONTINUED_AT);
    bool counts =
        continued && game_session_counts_for_statistics(game_port, &second);
    if (continued)
    {
        end_game_session(game_port, &second);
    }
    destroy_game_port(game_port);
    ASSERT_TRUE(captured);
    ASSERT_FALSE(saved.hint_used);
    ASSERT_TRUE(continued);
    ASSERT_TRUE(counts);
}

int main(void)
{
    RUN_TEST(test_init_game_session_starts_playing);
    RUN_TEST(test_init_game_session_generation_failure_owns_nothing);
    RUN_TEST(test_end_game_session_frees_once);
    RUN_TEST(test_handle_game_session_key_escape_leaves_at_once);
    RUN_TEST(test_handle_game_session_key_p_pauses);
    RUN_TEST(test_handle_game_session_key_board_keys_ignored_while_paused);
    RUN_TEST(test_handle_game_session_key_enter_while_paused_never_confirms);
    RUN_TEST(test_handle_game_session_key_resume_restores_board);
    RUN_TEST(test_handle_game_session_key_pause_freezes_timer);
    RUN_TEST(test_handle_game_session_key_prompt_freezes_timer);
    RUN_TEST(test_handle_game_session_key_menu_navigation);
    RUN_TEST(test_handle_game_session_key_new_game_without_progress);
    RUN_TEST(test_handle_game_session_key_new_game_with_progress_asks);
    RUN_TEST(test_handle_game_session_key_main_menu_without_progress_leaves);
    RUN_TEST(test_handle_game_session_key_main_menu_with_progress_asks);
    RUN_TEST(test_handle_game_session_key_abandon_without_progress_leaves);
    RUN_TEST(test_handle_game_session_key_abandon_with_progress_asks);
    RUN_TEST(test_handle_game_session_key_solved_board_ignores_edits);
    RUN_TEST(test_handle_game_session_key_solved_board_arrows_still_move);
    RUN_TEST(test_handle_game_session_key_solved_board_can_pause);
    RUN_TEST(test_handle_game_session_key_solved_board_new_game_without_asking);
    RUN_TEST(test_handle_game_session_key_solved_board_keeps_shortcuts);
    RUN_TEST(test_handle_game_session_key_completed_game_is_not_recorded_again);
    RUN_TEST(test_update_game_session_timer_completes_once_when_solved);
    RUN_TEST(test_handle_game_session_key_restart_button_never_asks);
    RUN_TEST(test_handle_game_session_key_new_game_failure_keeps_game);
    RUN_TEST(test_handle_game_session_key_unknown_keys_change_nothing);
    RUN_TEST(test_handle_game_session_key_game_shortcuts_ignored_while_paused);
    RUN_TEST(test_handle_game_session_key_undo_notice_cleared_by_next_key);
    RUN_TEST(test_handle_game_session_key_undo_reverts_the_latest_edit);
    RUN_TEST(test_init_game_session_hides_victory);
    RUN_TEST(test_update_game_session_timer_pending_or_wrong_shows_no_victory);
    RUN_TEST(test_update_game_session_timer_completion_opens_victory);
    RUN_TEST(test_update_game_session_timer_victory_opens_once);
    RUN_TEST(test_update_game_session_timer_paused_game_opens_no_victory);
    RUN_TEST(test_victory_summary_matches_the_finished_game);
    RUN_TEST(test_handle_game_session_key_victory_ignores_board_and_pause_keys);
    RUN_TEST(test_handle_game_session_key_victory_navigation);
    RUN_TEST(test_handle_game_session_key_victory_new_game);
    RUN_TEST(test_handle_game_session_key_victory_main_menu);
    RUN_TEST(
        test_handle_game_session_key_victory_new_game_failure_keeps_summary);
    RUN_TEST(test_handle_game_session_key_victory_unknown_keys_change_nothing);
    RUN_TEST(test_victory_again_after_new_game);
    RUN_TEST(test_handle_game_session_key_undo_keeps_the_move_count);
    RUN_TEST(test_restart_button_and_n_reset_moves_and_victory);
    RUN_TEST(test_init_game_session_uses_default_theme);
    RUN_TEST(test_set_game_session_theme_sets_and_defaults);
    RUN_TEST(test_handle_game_session_key_t_opens_theme_screen_and_pauses);
    RUN_TEST(test_theme_change_keeps_board_history_result_and_timer);
    RUN_TEST(test_theme_change_on_wrong_board_keeps_verdict);
    RUN_TEST(test_theme_change_on_solved_board_still_completes);
    RUN_TEST(test_theme_select_same_theme_is_continue);
    RUN_TEST(test_theme_screen_back_keeps_theme);
    RUN_TEST(test_theme_screen_escape_never_leaves_the_game);
    RUN_TEST(test_theme_screen_ignores_game_keys);
    RUN_TEST(test_theme_screen_navigation_does_not_change_the_theme);
    RUN_TEST(test_pause_menu_t_opens_theme_and_returns_to_menu);
    RUN_TEST(test_pause_menu_theme_change_returns_to_menu);
    RUN_TEST(test_theme_screen_reopens_marking_the_new_theme);
    RUN_TEST(test_victory_and_prompt_ignore_t);
    RUN_TEST(test_restart_keeps_session_theme);
    RUN_TEST(test_failed_new_game_keeps_session_theme);
    RUN_TEST(test_handle_game_session_key_s_in_the_pause_menu_asks_to_save);
    RUN_TEST(test_handle_game_session_key_save_request_keeps_the_selection);
    RUN_TEST(test_handle_game_session_key_s_while_playing_changes_nothing);
    RUN_TEST(test_handle_game_session_key_s_in_a_prompt_is_ignored);
    RUN_TEST(test_handle_game_session_key_resume_after_a_save_request);
    RUN_TEST(test_init_game_session_is_not_linked_to_a_save);
    RUN_TEST(test_init_game_session_from_save_restores_the_game);
    RUN_TEST(test_init_game_session_from_save_invalid_save_owns_nothing);
    RUN_TEST(test_restart_new_game_unlinks_a_continued_game);
    RUN_TEST(test_restart_button_unlinks_a_continued_game);
    RUN_TEST(test_confirmed_new_game_unlinks_a_continued_game);
    RUN_TEST(test_cancelled_new_game_keeps_a_continued_game_linked);
    RUN_TEST(test_failed_new_game_keeps_a_continued_game_linked);
    RUN_TEST(test_leaving_a_continued_game_does_not_unlink_it);
    RUN_TEST(test_capture_game_session_reads_the_timer_and_the_port);
    RUN_TEST(test_capture_game_session_solved_game_fails_untouched);
    RUN_TEST(test_continue_then_capture_returns_the_same_save);
    RUN_TEST(test_note_game_session_saved_links_the_session);
    RUN_TEST(test_note_game_session_saved_session_only_links_the_session);
    RUN_TEST(test_note_game_session_saved_failed_does_not_link);
    RUN_TEST(test_note_game_session_saved_failed_keeps_an_existing_link);
    RUN_TEST(test_save_notice_lasts_until_the_pause_menu_closes);
    RUN_TEST(test_handle_game_session_key_save_request_with_progress_no_prompt);
    RUN_TEST(test_init_game_session_from_save_ignores_the_time_since_the_save);
    RUN_TEST(test_a_continued_game_completes_once_with_restored_moves_and_time);
    RUN_TEST(
        test_handle_game_session_key_conflicting_entry_keeps_timer_and_pause);
    RUN_TEST(test_handle_game_session_key_h_shows_a_hint);
    RUN_TEST(test_handle_game_session_key_upper_h_shows_a_hint);
    RUN_TEST(test_handle_game_session_key_hint_changes_nothing_but_the_hint);
    RUN_TEST(test_handle_game_session_key_a_move_clears_the_hint);
    RUN_TEST(test_handle_game_session_key_a_group_edit_clears_the_hint);
    RUN_TEST(test_handle_game_session_key_a_cursor_move_keeps_the_hint);
    RUN_TEST(test_handle_game_session_key_an_ignored_key_keeps_the_hint);
    RUN_TEST(test_handle_game_session_key_a_successful_undo_clears_the_hint);
    RUN_TEST(test_handle_game_session_key_an_empty_undo_keeps_the_hint);
    RUN_TEST(test_handle_game_session_key_a_new_hint_replaces_the_old_one);
    RUN_TEST(test_handle_game_session_key_a_hint_does_not_pause_the_timer);
    RUN_TEST(test_handle_game_session_key_h_is_ignored_in_the_pause_menu);
    RUN_TEST(test_handle_game_session_key_h_is_ignored_on_the_theme_screen);
    RUN_TEST(test_handle_game_session_key_h_is_ignored_on_the_victory_screen);
    RUN_TEST(test_game_session_counts_for_statistics_false_after_a_hint);
    RUN_TEST(test_game_session_counts_for_statistics_stays_false_after_moves);
    RUN_TEST(test_game_session_conflict_reply_does_not_mark_the_game);
    RUN_TEST(test_game_session_full_board_reply_does_not_mark_the_game);
    RUN_TEST(test_game_session_solved_board_reply_does_not_mark_the_game);
    RUN_TEST(test_a_hinted_game_completes_but_does_not_count);
    RUN_TEST(test_an_unhinted_game_completes_and_counts);
    RUN_TEST(test_a_new_game_by_key_clears_the_hint_and_the_mark);
    RUN_TEST(test_the_restart_button_clears_the_hint_and_the_mark);
    RUN_TEST(test_a_confirmed_new_game_clears_the_hint_and_the_mark);
    RUN_TEST(test_a_failed_new_game_keeps_the_hint_and_the_mark);
    RUN_TEST(test_a_new_game_can_be_hinted_again);
    RUN_TEST(test_init_game_session_shows_no_hint);
    RUN_TEST(test_init_game_session_from_save_shows_no_hint);
    RUN_TEST(test_a_game_continued_from_a_hinted_save_does_not_count);
    RUN_TEST(test_the_hint_mark_survives_capturing_and_continuing);
    RUN_TEST(test_a_game_saved_without_a_hint_counts_when_continued);
    return test_harness_result();
}
