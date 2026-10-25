#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../test_harness.h"
#include "../game/fixture_puzzle.h"
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

    press(game_port, session, 'P', TEST_START_TIME + 30);
    ASSERT_TRUE(session->timer.paused);
    ASSERT_FALSE(update_game_session_timer(game_port, session, 500));
    press(game_port, session, TEST_ESCAPE, 600);
    ASSERT_FALSE(session->timer.paused);
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
    return test_harness_result();
}
