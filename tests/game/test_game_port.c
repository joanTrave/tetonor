#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../test_harness.h"
#include "fixture_puzzle.h"
#include "../../src/game/game_port.h"

#define TEST_DAILY_EPOCH ((time_t)1793059200) /* 2026-10-27 00:00:00 UTC */
#define TEST_SECONDS_PER_DAY 86400

#define TEST_SEED 42
#define TEST_BOXES_PER_GROUP 3
#define TEST_MAX_FREED 4
#define TEST_OLD_HIDDEN_COUNT 4
#define TEST_NEW_HIDDEN_COUNT 3
#define TEST_TYPED_VALUE 7
#define TEST_MESSY_GROUP_CELL 5
#define TEST_LEVEL_COUNT 3
#define TEST_MEDIUM_HIDDEN_COUNT 8

static const int fixture_old_hidden[TEST_OLD_HIDDEN_COUNT] = {2, 5, 6, 15};
static const int fixture_new_hidden[TEST_NEW_HIDDEN_COUNT] = {0, 7, 9};

/* Fakes for the generation functions of the port. */
static int fake_generate_calls;
static int fake_generate_difficulty;
static bool fake_generate_fails;
static const int* fake_hidden;
static size_t fake_hidden_count;
static int fake_free_calls;
static uintptr_t fake_freed[TEST_MAX_FREED];

/* Fake for the daily generation function of the port. Always returns the
 * same fixed shape (fixture_old_hidden), so two calls are byte-identical: the
 * same stand-in a "replay" expects of the real generator on a frozen date. */
static int fake_generate_daily_calls;
static CalendarDate fake_generate_daily_date;
static bool fake_generate_daily_fails;

static NumberGeneration* fake_generate_daily(CalendarDate date)
{
    fake_generate_daily_calls++;
    fake_generate_daily_date = date;
    if (fake_generate_daily_fails)
    {
        return NULL;
    }
    NumberGeneration* puzzle = malloc(sizeof *puzzle);
    if (puzzle != NULL)
    {
        build_fixture_puzzle(puzzle, fixture_old_hidden, TEST_OLD_HIDDEN_COUNT);
    }
    return puzzle;
}

static void reset_fakes(void)
{
    fake_generate_calls       = 0;
    fake_generate_difficulty  = -1;
    fake_generate_fails       = false;
    fake_hidden               = fixture_old_hidden;
    fake_hidden_count         = TEST_OLD_HIDDEN_COUNT;
    fake_free_calls           = 0;
    fake_generate_daily_calls = 0;
    fake_generate_daily_date  = (CalendarDate){0, 0, 0};
    fake_generate_daily_fails = false;
    memset(fake_freed, 0, sizeof fake_freed);
}

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
        build_fixture_puzzle(puzzle, fake_hidden, fake_hidden_count);
    }
    return puzzle;
}

static void fake_free(NumberGeneration* puzzle)
{
    if (fake_free_calls < TEST_MAX_FREED)
    {
        fake_freed[fake_free_calls] = (uintptr_t)puzzle;
    }
    fake_free_calls++;
    free(puzzle);
}

static int count_hidden(const NumberGeneration* number_generation)
{
    int hidden = 0;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (number_generation->available_array[i] == TETONOR_HIDDEN_NUMBER)
        {
            hidden++;
        }
    }
    return hidden;
}

static void test_create_game_port_returns_non_null(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    destroy_game_port(game_port);
}

static void test_create_game_port_sets_default_difficulty(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    int difficulty = game_port->difficulty;
    destroy_game_port(game_port);
    ASSERT_EQ_INT(difficulty, TETONOR_DEFAULT_DIFFICULTY);
    ASSERT_EQ_INT(TETONOR_DEFAULT_DIFFICULTY, DIFFICULTY_MEDIUM);
}

static void test_create_game_port_wires_number_generation_functions(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    int generate_wired =
        game_port->generate_random_numbers_f == generate_random_numbers;
    int free_wired =
        game_port->free_number_generation_f == free_number_generation;
    destroy_game_port(game_port);
    ASSERT_TRUE(generate_wired);
    ASSERT_TRUE(free_wired);
}

static void test_game_port_generates_and_frees_through_port(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    srand(TEST_SEED);
    NumberGeneration* number_generation =
        game_port->generate_random_numbers_f(game_port->difficulty);
    if (number_generation == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(number_generation);
    }
    int hidden = count_hidden(number_generation);
    game_port->free_number_generation_f(number_generation);
    destroy_game_port(game_port);
    ASSERT_EQ_INT(hidden, TEST_MEDIUM_HIDDEN_COUNT);
}

static void test_destroy_game_port_accepts_null(void)
{
    destroy_game_port(NULL);
    ASSERT_TRUE(1);
}

static void press_digit_through_port(const GamePort* game_port, Board* board,
                                     int digit)
{
    BoardInput input = {.kind = BOARD_INPUT_DIGIT, .digit = digit};
    game_port->apply_board_input_f(board, input);
}

static void press_through_port(const GamePort* game_port, Board* board,
                               BoardInputKind kind)
{
    BoardInput input = {.kind = kind};
    game_port->apply_board_input_f(board, input);
}

static void press_operation_through_port(const GamePort* game_port,
                                         Board* board, GroupOperation operation)
{
    BoardInput input = {.kind = BOARD_INPUT_OPERATION, .operation = operation};
    game_port->apply_board_input_f(board, input);
}

static void type_value_through_port(const GamePort* game_port, Board* board,
                                    int value)
{
    if (value >= TETONOR_DIGIT_BASE)
    {
        press_digit_through_port(game_port, board, value / TETONOR_DIGIT_BASE);
    }
    press_digit_through_port(game_port, board, value % TETONOR_DIGIT_BASE);
}

static void enter_group_through_port(const GamePort* game_port, Board* board,
                                     const OperandGroup* group)
{
    type_value_through_port(game_port, board, group->smaller);
    press_through_port(game_port, board, BOARD_INPUT_MOVE_RIGHT);
    press_operation_through_port(game_port, board, group->operation);
    press_through_port(game_port, board, BOARD_INPUT_MOVE_RIGHT);
    type_value_through_port(game_port, board, group->greater);
}

/* Starts on the first box of the first group and leaves the focus on the
 * greater box of the last group. RIGHT never wraps, so a row is left by DOWN
 * and walked back to its first box by LEFT. */
static void enter_groups_through_port(const GamePort* game_port, Board* board,
                                      const OperandGroup* groups)
{
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        enter_group_through_port(game_port, board, &groups[cell]);
        if (cell == TETONOR_CELL_COUNT - 1)
        {
            continue;
        }
        if (cell % TETONOR_GRID_SIZE != TETONOR_GRID_SIZE - 1)
        {
            press_through_port(game_port, board, BOARD_INPUT_MOVE_RIGHT);
            continue;
        }
        press_through_port(game_port, board, BOARD_INPUT_MOVE_DOWN);
        for (int step = 0; step < TETONOR_GRID_SIZE * TEST_BOXES_PER_GROUP - 1;
             step++)
        {
            press_through_port(game_port, board, BOARD_INPUT_MOVE_LEFT);
        }
    }
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

/* Types the generator's own groups, then every hidden slot's solution value,
 * adding first_slot_offset to the first one (modulo the value range) to make
 * the answer wrong on demand. UP from the slots reaches the groups, DOWN from
 * the last group returns to the first slot, and the RIGHT after the last slot
 * focuses the submit control, which ACTIVATE then presses. */
static void play_session(const GamePort* game_port, Board* board,
                         const NumberGeneration* puzzle, int first_slot_offset)
{
    press_through_port(game_port, board, BOARD_INPUT_MOVE_UP);
    enter_groups_through_port(game_port, board, puzzle->solution_groups);
    press_through_port(game_port, board, BOARD_INPUT_MOVE_DOWN);
    bool first = true;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (puzzle->available_array[i] != TETONOR_HIDDEN_NUMBER)
        {
            continue;
        }
        int value = puzzle->solution_array[i];
        if (first)
        {
            value = (value + first_slot_offset) % (TETONOR_MAX_VALUE + 1);
            first = false;
        }
        type_value_through_port(game_port, board, value);
        press_through_port(game_port, board, BOARD_INPUT_MOVE_RIGHT);
    }
    press_through_port(game_port, board, BOARD_INPUT_ACTIVATE);
}

static void test_create_game_port_wires_board_functions(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    int init_wired  = game_port->init_board_f == init_board;
    int apply_wired = game_port->apply_board_input_f == apply_board_input;
    destroy_game_port(game_port);
    ASSERT_TRUE(init_wired);
    ASSERT_TRUE(apply_wired);
}

static void test_game_port_board_session_through_port_is_correct(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    srand(TEST_SEED);
    NumberGeneration* number_generation =
        game_port->generate_random_numbers_f(game_port->difficulty);
    if (number_generation == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(number_generation);
    }
    Board board;
    game_port->init_board_f(&board, number_generation);
    BoardResult before = board.result;
    play_session(game_port, &board, number_generation, 0);
    BoardResult after = board.result;
    game_port->free_number_generation_f(number_generation);
    destroy_game_port(game_port);
    ASSERT_EQ_INT(before, BOARD_RESULT_PENDING);
    ASSERT_EQ_INT(after, BOARD_RESULT_CORRECT);
}

static void test_game_port_board_session_through_port_is_wrong(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    srand(TEST_SEED);
    NumberGeneration* number_generation =
        game_port->generate_random_numbers_f(game_port->difficulty);
    if (number_generation == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(number_generation);
    }
    Board board;
    game_port->init_board_f(&board, number_generation);
    play_session(game_port, &board, number_generation, 1);
    BoardResult result = board.result;
    game_port->free_number_generation_f(number_generation);
    destroy_game_port(game_port);
    ASSERT_EQ_INT(result, BOARD_RESULT_WRONG);
}

static NumberGeneration* generate_through_port(const GamePort* game_port)
{
    srand(TEST_SEED);
    return game_port->generate_random_numbers_f(game_port->difficulty);
}

static void test_game_port_groups_typed_through_port_match_solution(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    NumberGeneration* number_generation = generate_through_port(game_port);
    if (number_generation == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(number_generation);
    }
    Board board;
    game_port->init_board_f(&board, number_generation);
    press_through_port(game_port, &board, BOARD_INPUT_MOVE_UP);
    enter_groups_through_port(game_port, &board,
                              number_generation->solution_groups);
    bool matches =
        groups_match(board.groups, number_generation->solution_groups);
    BoardFocus focus   = board.focus;
    BoardResult result = board.result;
    game_port->free_number_generation_f(number_generation);
    destroy_game_port(game_port);
    ASSERT_TRUE(matches);
    ASSERT_EQ_INT(focus, BOARD_FOCUS_GROUP);
    ASSERT_EQ_INT(result, BOARD_RESULT_PENDING);
}

static void test_game_port_board_session_without_groups_stays_pending(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    NumberGeneration* number_generation = generate_through_port(game_port);
    if (number_generation == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(number_generation);
    }
    Board board;
    game_port->init_board_f(&board, number_generation);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (number_generation->available_array[i] == TETONOR_HIDDEN_NUMBER)
        {
            type_value_through_port(game_port, &board,
                                    number_generation->solution_array[i]);
            press_through_port(game_port, &board, BOARD_INPUT_MOVE_RIGHT);
        }
    }
    press_through_port(game_port, &board, BOARD_INPUT_ACTIVATE);
    BoardResult result = board.result;
    game_port->free_number_generation_f(number_generation);
    destroy_game_port(game_port);
    ASSERT_EQ_INT(result, BOARD_RESULT_PENDING);
}

/* Field by field: padding bytes of a Board are unspecified. */
static bool boards_match(const Board* first, const Board* second)
{
    if (first->puzzle != second->puzzle || first->cursor != second->cursor ||
        first->group_cursor != second->group_cursor ||
        first->group_field != second->group_field ||
        first->focus != second->focus || first->result != second->result)
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

/* True when board is exactly what init_board makes over its own puzzle. */
static bool board_is_fresh(const Board* board)
{
    Board expected;
    init_board(&expected, board->puzzle);
    return boards_match(board, &expected);
}

/* A port whose generation functions are the fakes above. */
static GamePort* create_fake_port(void)
{
    reset_fakes();
    GamePort* game_port = create_game_port();
    if (game_port != NULL)
    {
        game_port->generate_random_numbers_f  = &fake_generate;
        game_port->free_number_generation_f   = &fake_free;
        game_port->generate_daily_challenge_f = &fake_generate_daily;
    }
    return game_port;
}

/* Generates the first puzzle through the fake port and starts a board over it.
 * The counters are reset afterwards, so tests count only the restarts. */
static NumberGeneration* start_fake_game(const GamePort* game_port,
                                         Board* board)
{
    NumberGeneration* puzzle =
        game_port->generate_random_numbers_f(game_port->difficulty);
    if (puzzle != NULL)
    {
        game_port->init_board_f(board, puzzle);
    }
    fake_generate_calls      = 0;
    fake_generate_difficulty = -1;
    return puzzle;
}

/* Typed slot, typed group boxes, a remembered cursor and corrupt derived data,
 * with the focus on the restart control. */
static void mess_up_board(Board* board)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (board->slots[i].editable)
        {
            board->slots[i].value = TEST_TYPED_VALUE;
        }
        board->slot_usages[i] = SLOT_USAGE_TWICE;
    }
    board->groups[TEST_MESSY_GROUP_CELL] =
        fixture_groups[TEST_MESSY_GROUP_CELL];
    board->group_cursor = TEST_MESSY_GROUP_CELL;
    board->group_field  = GROUP_FIELD_GREATER;
    board->focus        = BOARD_FOCUS_RESTART;
}

static void test_restart_game_replaces_puzzle_and_frees_old_once(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    Board board;
    NumberGeneration* puzzle = start_fake_game(game_port, &board);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    uintptr_t old_address = (uintptr_t)puzzle;
    bool restarted        = restart_game(game_port, &board, &puzzle);
    bool replaced         = (uintptr_t)puzzle != old_address;
    bool borrowed         = board.puzzle == puzzle;
    int difficulty        = game_port->difficulty;
    int free_calls        = fake_free_calls;
    uintptr_t freed       = fake_freed[0];
    int generate_calls    = fake_generate_calls;
    int generated_with    = fake_generate_difficulty;
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_TRUE(restarted);
    ASSERT_TRUE(replaced);
    ASSERT_TRUE(borrowed);
    ASSERT_EQ_INT(free_calls, 1);
    ASSERT_TRUE(freed == old_address);
    ASSERT_EQ_INT(generate_calls, 1);
    ASSERT_EQ_INT(generated_with, difficulty);
}

static void test_restart_game_resets_board_to_initial_state(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    Board board;
    NumberGeneration* puzzle = start_fake_game(game_port, &board);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    mess_up_board(&board);
    board.result       = BOARD_RESULT_WRONG;
    bool restarted     = restart_game(game_port, &board, &puzzle);
    bool fresh         = board_is_fresh(&board);
    BoardResult result = board.result;
    BoardFocus focus   = board.focus;
    bool groups_empty  = true;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        groups_empty = groups_empty &&
                       board.groups[i].smaller == TETONOR_EMPTY_ENTRY &&
                       board.groups[i].greater == TETONOR_EMPTY_ENTRY &&
                       board.groups[i].operation == GROUP_OPERATION_NONE;
        groups_empty =
            groups_empty && (!board.slots[i].editable ||
                             board.slots[i].value == TETONOR_EMPTY_ENTRY);
    }
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_TRUE(restarted);
    ASSERT_TRUE(fresh);
    ASSERT_TRUE(groups_empty);
    ASSERT_EQ_INT(result, BOARD_RESULT_PENDING);
    ASSERT_EQ_INT(focus, BOARD_FOCUS_SLOT);
}

/* Plays a full session (offset 0 is correct, 1 is wrong), moves the focus to
 * the restart control and presses it, as the UI does. */
static void play_and_press_restart(const GamePort* game_port, Board* board,
                                   const NumberGeneration* puzzle, int offset,
                                   BoardResult expected, bool* restart_event,
                                   bool* judged_as_expected)
{
    play_session(game_port, board, puzzle, offset);
    *judged_as_expected = board->result == expected;
    press_through_port(game_port, board, BOARD_INPUT_MOVE_DOWN);
    BoardInput activate = {.kind = BOARD_INPUT_ACTIVATE};
    *restart_event =
        game_port->apply_board_input_f(board, activate) == BOARD_EVENT_RESTART;
}

static void assert_restart_after_result(int offset, BoardResult expected)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    Board board;
    NumberGeneration* puzzle = start_fake_game(game_port, &board);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    bool restart_event;
    bool judged;
    play_and_press_restart(game_port, &board, puzzle, offset, expected,
                           &restart_event, &judged);
    bool restarted = restart_event && restart_game(game_port, &board, &puzzle);
    bool fresh     = board_is_fresh(&board);
    BoardResult result = board.result;
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_TRUE(judged);
    ASSERT_TRUE(restart_event);
    ASSERT_TRUE(restarted);
    ASSERT_TRUE(fresh);
    ASSERT_EQ_INT(result, BOARD_RESULT_PENDING);
}

static void test_restart_game_after_correct_result_starts_pending(void)
{
    assert_restart_after_result(0, BOARD_RESULT_CORRECT);
}

static void test_restart_game_after_wrong_result_starts_pending(void)
{
    assert_restart_after_result(1, BOARD_RESULT_WRONG);
}

static void test_restart_game_before_any_answer_starts_pending(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    Board board;
    NumberGeneration* puzzle = start_fake_game(game_port, &board);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    press_through_port(game_port, &board, BOARD_INPUT_MOVE_DOWN);
    press_through_port(game_port, &board, BOARD_INPUT_MOVE_DOWN);
    BoardFocus focus_before = board.focus;
    BoardInput activate     = {.kind = BOARD_INPUT_ACTIVATE};
    BoardEvent event        = game_port->apply_board_input_f(&board, activate);
    bool restarted          = event == BOARD_EVENT_RESTART &&
                              restart_game(game_port, &board, &puzzle);
    bool fresh              = board_is_fresh(&board);
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_EQ_INT(focus_before, BOARD_FOCUS_RESTART);
    ASSERT_TRUE(restarted);
    ASSERT_TRUE(fresh);
}

static void test_restart_game_uses_new_puzzle_numbers(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    Board board;
    NumberGeneration* puzzle = start_fake_game(game_port, &board);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    bool old_editable_before     = board.slots[fixture_old_hidden[0]].editable;
    fake_hidden                  = fixture_new_hidden;
    fake_hidden_count            = TEST_NEW_HIDDEN_COUNT;
    bool restarted               = restart_game(game_port, &board, &puzzle);
    bool slots_follow_new_puzzle = true;
    int editable_count           = 0;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        slots_follow_new_puzzle =
            slots_follow_new_puzzle &&
            board.slots[i].editable ==
                (puzzle->available_array[i] == TETONOR_HIDDEN_NUMBER);
        editable_count += board.slots[i].editable ? 1 : 0;
    }
    bool old_editable_after = board.slots[fixture_old_hidden[0]].editable;
    int cursor              = board.cursor;
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_TRUE(restarted);
    ASSERT_TRUE(old_editable_before);
    ASSERT_FALSE(old_editable_after);
    ASSERT_TRUE(slots_follow_new_puzzle);
    ASSERT_EQ_INT(editable_count, TEST_NEW_HIDDEN_COUNT);
    ASSERT_EQ_INT(cursor, fixture_new_hidden[0]);
}

static void test_restart_game_generation_failure_keeps_game(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    Board board;
    NumberGeneration* puzzle = start_fake_game(game_port, &board);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    mess_up_board(&board);
    board.result                 = BOARD_RESULT_WRONG;
    Board before                 = board;
    NumberGeneration* old_puzzle = puzzle;
    fake_generate_fails          = true;
    bool restarted               = restart_game(game_port, &board, &puzzle);
    bool same_puzzle             = puzzle == old_puzzle;
    bool same_board              = boards_match(&board, &before);
    int free_calls               = fake_free_calls;
    int generate_calls           = fake_generate_calls;
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_FALSE(restarted);
    ASSERT_TRUE(same_puzzle);
    ASSERT_TRUE(same_board);
    ASSERT_EQ_INT(free_calls, 0);
    ASSERT_EQ_INT(generate_calls, 1);
}

static void test_restart_game_after_failure_can_restart_again(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    Board board;
    NumberGeneration* puzzle = start_fake_game(game_port, &board);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    uintptr_t old_address = (uintptr_t)puzzle;
    fake_generate_fails   = true;
    bool failed           = !restart_game(game_port, &board, &puzzle);
    fake_generate_fails   = false;
    bool restarted        = restart_game(game_port, &board, &puzzle);
    bool borrowed         = board.puzzle == puzzle;
    int free_calls        = fake_free_calls;
    uintptr_t freed       = fake_freed[0];
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_TRUE(failed);
    ASSERT_TRUE(restarted);
    ASSERT_TRUE(borrowed);
    ASSERT_EQ_INT(free_calls, 1);
    ASSERT_TRUE(freed == old_address);
}

static void test_restart_game_twice_frees_each_old_puzzle_once(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    Board board;
    NumberGeneration* puzzle = start_fake_game(game_port, &board);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    uintptr_t first_address  = (uintptr_t)puzzle;
    bool first_restart       = restart_game(game_port, &board, &puzzle);
    uintptr_t second_address = (uintptr_t)puzzle;
    bool second_restart      = restart_game(game_port, &board, &puzzle);
    bool borrowed            = board.puzzle == puzzle;
    int free_calls           = fake_free_calls;
    uintptr_t first_freed    = fake_freed[0];
    uintptr_t second_freed   = fake_freed[1];
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_TRUE(first_restart);
    ASSERT_TRUE(second_restart);
    ASSERT_TRUE(borrowed);
    ASSERT_EQ_INT(free_calls, 2);
    ASSERT_TRUE(first_freed == first_address);
    ASSERT_TRUE(second_freed == second_address);
}

static void test_restart_game_with_real_port_replaces_puzzle(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    NumberGeneration* puzzle = generate_through_port(game_port);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    Board board;
    game_port->init_board_f(&board, puzzle);
    mess_up_board(&board);
    uintptr_t old_address = (uintptr_t)puzzle;
    bool restarted        = restart_game(game_port, &board, &puzzle);
    bool replaced         = (uintptr_t)puzzle != old_address;
    bool borrowed         = board.puzzle == puzzle;
    bool fresh            = board_is_fresh(&board);
    int hidden            = restarted ? count_hidden(puzzle) : 0;
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_TRUE(restarted);
    ASSERT_TRUE(replaced);
    ASSERT_TRUE(borrowed);
    ASSERT_TRUE(fresh);
    ASSERT_TRUE(hidden >= 1);
}

static const Difficulty test_levels[TEST_LEVEL_COUNT] = {
    DIFFICULTY_EASY, DIFFICULTY_MEDIUM, DIFFICULTY_HARD};
static const int test_hidden_counts[TEST_LEVEL_COUNT] = {4, 8, 12};

static void test_game_port_difficulty_reaches_generation_per_level(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        game_port->difficulty    = test_levels[level];
        NumberGeneration* puzzle = generate_through_port(game_port);
        if (puzzle == NULL)
        {
            destroy_game_port(game_port);
            ASSERT_NOT_NULL(puzzle);
        }
        int hidden = count_hidden(puzzle);
        game_port->free_number_generation_f(puzzle);
        if (hidden != test_hidden_counts[level])
        {
            destroy_game_port(game_port);
            ASSERT_EQ_INT(hidden, test_hidden_counts[level]);
        }
    }
    destroy_game_port(game_port);
}

static void test_game_port_board_session_is_correct_per_level(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        game_port->difficulty    = test_levels[level];
        NumberGeneration* puzzle = generate_through_port(game_port);
        if (puzzle == NULL)
        {
            destroy_game_port(game_port);
            ASSERT_NOT_NULL(puzzle);
        }
        Board board;
        game_port->init_board_f(&board, puzzle);
        play_session(game_port, &board, puzzle, 0);
        BoardResult result = board.result;
        game_port->free_number_generation_f(puzzle);
        if (result != BOARD_RESULT_CORRECT)
        {
            destroy_game_port(game_port);
            ASSERT_EQ_INT(result, BOARD_RESULT_CORRECT);
        }
    }
    destroy_game_port(game_port);
}

static void test_restart_game_keeps_selected_difficulty(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    game_port->difficulty = DIFFICULTY_HARD;
    Board board;
    NumberGeneration* puzzle = start_fake_game(game_port, &board);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    bool restarted     = restart_game(game_port, &board, &puzzle);
    int generated_with = fake_generate_difficulty;
    int kept           = game_port->difficulty;
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_TRUE(restarted);
    ASSERT_EQ_INT(generated_with, DIFFICULTY_HARD);
    ASSERT_EQ_INT(kept, DIFFICULTY_HARD);
}

static void test_restart_game_with_real_port_keeps_level(void)
{
    const Difficulty levels[] = {DIFFICULTY_EASY, DIFFICULTY_HARD};
    const int expected[]      = {4, 12};
    for (size_t i = 0; i < sizeof levels / sizeof levels[0]; i++)
    {
        GamePort* game_port = create_game_port();
        ASSERT_NOT_NULL(game_port);
        game_port->difficulty    = levels[i];
        NumberGeneration* puzzle = generate_through_port(game_port);
        if (puzzle == NULL)
        {
            destroy_game_port(game_port);
            ASSERT_NOT_NULL(puzzle);
        }
        Board board;
        game_port->init_board_f(&board, puzzle);
        bool restarted = restart_game(game_port, &board, &puzzle);
        int hidden     = restarted ? count_hidden(puzzle) : 0;
        game_port->free_number_generation_f(puzzle);
        destroy_game_port(game_port);
        ASSERT_TRUE(restarted);
        ASSERT_EQ_INT(hidden, expected[i]);
    }
}

static void test_create_game_port_does_not_inherit_previous_difficulty(void)
{
    GamePort* first = create_game_port();
    ASSERT_NOT_NULL(first);
    first->difficulty = DIFFICULTY_HARD;
    destroy_game_port(first);
    GamePort* second = create_game_port();
    ASSERT_NOT_NULL(second);
    int difficulty = second->difficulty;
    destroy_game_port(second);
    ASSERT_EQ_INT(difficulty, DIFFICULTY_MEDIUM);
}

static void test_create_game_port_wires_undo(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    bool undo_wired = game_port->undo_board_move_f == undo_board_move;
    destroy_game_port(game_port);
    ASSERT_TRUE(undo_wired);
}

static void test_game_port_undo_through_port_reverts_move(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    NumberGeneration puzzle;
    build_fixture_puzzle(&puzzle, fixture_old_hidden, TEST_OLD_HIDDEN_COUNT);
    Board board;
    game_port->init_board_f(&board, &puzzle);
    int first_slot = fixture_old_hidden[0];
    press_digit_through_port(game_port, &board, TEST_TYPED_VALUE);
    int typed        = board.slots[first_slot].value;
    bool undone      = game_port->undo_board_move_f(&board);
    int reverted     = board.slots[first_slot].value;
    bool undone_more = game_port->undo_board_move_f(&board);
    destroy_game_port(game_port);
    ASSERT_EQ_INT(typed, TEST_TYPED_VALUE);
    ASSERT_TRUE(undone);
    ASSERT_EQ_INT(reverted, TETONOR_EMPTY_ENTRY);
    ASSERT_FALSE(undone_more);
}

static void test_restart_game_empties_move_history(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    Board board;
    NumberGeneration* puzzle = start_fake_game(game_port, &board);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    press_digit_through_port(game_port, &board, TEST_TYPED_VALUE);
    Board with_move  = board;
    bool had_history = game_port->undo_board_move_f(&with_move);
    bool restarted   = restart_game(game_port, &board, &puzzle);
    bool undone      = game_port->undo_board_move_f(&board);
    bool fresh       = board_is_fresh(&board);
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_TRUE(had_history);
    ASSERT_TRUE(restarted);
    ASSERT_FALSE(undone);
    ASSERT_TRUE(fresh);
}

static void test_restart_game_failure_keeps_move_history(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    Board board;
    NumberGeneration* puzzle = start_fake_game(game_port, &board);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    int first_slot = fixture_old_hidden[0];
    press_digit_through_port(game_port, &board, TEST_TYPED_VALUE);
    fake_generate_fails = true;
    bool restarted      = restart_game(game_port, &board, &puzzle);
    bool undone         = game_port->undo_board_move_f(&board);
    int reverted        = board.slots[first_slot].value;
    bool undone_more    = game_port->undo_board_move_f(&board);
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_FALSE(restarted);
    ASSERT_TRUE(undone);
    ASSERT_EQ_INT(reverted, TETONOR_EMPTY_ENTRY);
    ASSERT_FALSE(undone_more);
}

static void test_restart_game_with_real_port_empties_move_history(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    NumberGeneration* puzzle = generate_through_port(game_port);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    Board board;
    game_port->init_board_f(&board, puzzle);
    press_digit_through_port(game_port, &board, TEST_TYPED_VALUE);
    bool restarted = restart_game(game_port, &board, &puzzle);
    bool undone    = game_port->undo_board_move_f(&board);
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_TRUE(restarted);
    ASSERT_FALSE(undone);
}

static void test_create_game_port_wires_timer_functions(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    bool start_wired  = game_port->start_game_timer_f == start_game_timer;
    bool update_wired = game_port->update_game_timer_f == update_game_timer;
    destroy_game_port(game_port);
    ASSERT_TRUE(start_wired);
    ASSERT_TRUE(update_wired);
}

static void test_game_port_timer_through_port_completes_once(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    GameTimer timer;
    game_port->start_game_timer_f(&timer, 100);
    bool pending_completes =
        game_port->update_game_timer_f(&timer, BOARD_RESULT_PENDING, 105);
    bool first_completes =
        game_port->update_game_timer_f(&timer, BOARD_RESULT_CORRECT, 112);
    bool second_completes =
        game_port->update_game_timer_f(&timer, BOARD_RESULT_CORRECT, 200);
    destroy_game_port(game_port);
    ASSERT_FALSE(pending_completes);
    ASSERT_TRUE(first_completes);
    ASSERT_FALSE(second_completes);
    ASSERT_EQ_INT(timer.elapsed_seconds, 12);
}

static void test_create_game_port_wires_pause_and_resume(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    bool pause_wired  = game_port->pause_game_timer_f == pause_game_timer;
    bool resume_wired = game_port->resume_game_timer_f == resume_game_timer;
    destroy_game_port(game_port);
    ASSERT_TRUE(pause_wired);
    ASSERT_TRUE(resume_wired);
}

static void test_create_game_port_wires_lock_and_progress(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    bool lock_wired     = game_port->is_board_locked_f == is_board_locked;
    bool progress_wired = game_port->board_progress_would_be_lost_f ==
                          board_progress_would_be_lost;
    destroy_game_port(game_port);
    ASSERT_TRUE(lock_wired);
    ASSERT_TRUE(progress_wired);
}

static void test_game_port_pause_through_port_freezes_time(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    GameTimer timer;
    game_port->start_game_timer_f(&timer, 100);
    game_port->pause_game_timer_f(&timer, 105);
    bool paused_update =
        game_port->update_game_timer_f(&timer, BOARD_RESULT_PENDING, 500);
    int32_t frozen = timer.elapsed_seconds;
    game_port->resume_game_timer_f(&timer, 600);
    bool resumed_update =
        game_port->update_game_timer_f(&timer, BOARD_RESULT_PENDING, 603);
    destroy_game_port(game_port);
    ASSERT_FALSE(paused_update);
    ASSERT_EQ_INT(frozen, 5);
    ASSERT_FALSE(resumed_update);
    ASSERT_EQ_INT(timer.elapsed_seconds, 8);
}

static void test_game_port_correct_board_through_port_is_locked(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    NumberGeneration* puzzle = generate_through_port(game_port);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    Board board;
    game_port->init_board_f(&board, puzzle);
    bool locked_before = game_port->is_board_locked_f(&board);
    play_session(game_port, &board, puzzle, 0);
    bool correct = board.result == BOARD_RESULT_CORRECT;
    bool locked  = game_port->is_board_locked_f(&board);
    bool at_risk = game_port->board_progress_would_be_lost_f(&board);
    Board before = board;
    press_through_port(game_port, &board, BOARD_INPUT_MOVE_UP);
    press_digit_through_port(game_port, &board, TEST_TYPED_VALUE);
    press_through_port(game_port, &board, BOARD_INPUT_DELETE);
    bool undone         = game_port->undo_board_move_f(&board);
    before.focus        = board.focus;
    before.cursor       = board.cursor;
    before.group_cursor = board.group_cursor;
    before.group_field  = board.group_field;
    bool unchanged      = boards_match(&board, &before) &&
                          board.history.count == before.history.count;
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_FALSE(locked_before);
    ASSERT_TRUE(correct);
    ASSERT_TRUE(locked);
    ASSERT_FALSE(at_risk);
    ASSERT_FALSE(undone);
    ASSERT_TRUE(unchanged);
}

static void test_restart_game_unlocks_the_board(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    Board board;
    NumberGeneration* puzzle = start_fake_game(game_port, &board);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    play_session(game_port, &board, puzzle, 0);
    bool locked_before = game_port->is_board_locked_f(&board);
    bool restarted     = restart_game(game_port, &board, &puzzle);
    bool locked_after  = game_port->is_board_locked_f(&board);
    bool at_risk       = game_port->board_progress_would_be_lost_f(&board);
    bool fresh         = board_is_fresh(&board);
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_TRUE(locked_before);
    ASSERT_TRUE(restarted);
    ASSERT_FALSE(locked_after);
    ASSERT_FALSE(at_risk);
    ASSERT_TRUE(fresh);
}

static void test_create_game_port_wires_move_count(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    bool wired = game_port->board_move_count_f == board_move_count;
    destroy_game_port(game_port);
    ASSERT_TRUE(wired);
}

static void test_game_port_counts_moves_through_port(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    NumberGeneration puzzle;
    build_fixture_puzzle(&puzzle, fixture_old_hidden, TEST_OLD_HIDDEN_COUNT);
    Board board;
    game_port->init_board_f(&board, &puzzle);
    int fresh = game_port->board_move_count_f(&board);
    press_digit_through_port(game_port, &board, TEST_TYPED_VALUE);
    int after_digit = game_port->board_move_count_f(&board);
    press_through_port(game_port, &board, BOARD_INPUT_DELETE);
    press_through_port(game_port, &board, BOARD_INPUT_DELETE);
    int after_deletes = game_port->board_move_count_f(&board);
    press_digit_through_port(game_port, &board, 1);
    press_digit_through_port(game_port, &board, 2);
    press_digit_through_port(game_port, &board, 9);
    press_through_port(game_port, &board, BOARD_INPUT_MOVE_RIGHT);
    press_through_port(game_port, &board, BOARD_INPUT_ACTIVATE);
    int after_more = game_port->board_move_count_f(&board);
    bool undone    = game_port->undo_board_move_f(&board);
    bool undone_2  = game_port->undo_board_move_f(&board);
    int after_undo = game_port->board_move_count_f(&board);
    destroy_game_port(game_port);
    ASSERT_EQ_INT(fresh, 0);
    ASSERT_EQ_INT(after_digit, 1);
    ASSERT_EQ_INT(after_deletes, 2);
    ASSERT_EQ_INT(after_more, 4);
    ASSERT_TRUE(undone);
    ASSERT_TRUE(undone_2);
    ASSERT_EQ_INT(after_undo, 4);
}

static void test_restart_game_resets_move_count(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    Board board;
    NumberGeneration* puzzle = start_fake_game(game_port, &board);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    press_digit_through_port(game_port, &board, TEST_TYPED_VALUE);
    press_through_port(game_port, &board, BOARD_INPUT_MOVE_RIGHT);
    press_digit_through_port(game_port, &board, TEST_TYPED_VALUE);
    int before     = game_port->board_move_count_f(&board);
    bool restarted = restart_game(game_port, &board, &puzzle);
    int after      = game_port->board_move_count_f(&board);
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_EQ_INT(before, 2);
    ASSERT_TRUE(restarted);
    ASSERT_EQ_INT(after, 0);
}

static void test_restart_game_failure_keeps_move_count(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    Board board;
    NumberGeneration* puzzle = start_fake_game(game_port, &board);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    press_digit_through_port(game_port, &board, TEST_TYPED_VALUE);
    fake_generate_fails = true;
    bool restarted      = restart_game(game_port, &board, &puzzle);
    int after           = game_port->board_move_count_f(&board);
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_FALSE(restarted);
    ASSERT_EQ_INT(after, 1);
}

static void test_restart_game_with_real_port_resets_move_count(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    NumberGeneration* puzzle = generate_through_port(game_port);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    Board board;
    game_port->init_board_f(&board, puzzle);
    press_digit_through_port(game_port, &board, TEST_TYPED_VALUE);
    int before     = game_port->board_move_count_f(&board);
    bool restarted = restart_game(game_port, &board, &puzzle);
    int after      = game_port->board_move_count_f(&board);
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_EQ_INT(before, 1);
    ASSERT_TRUE(restarted);
    ASSERT_EQ_INT(after, 0);
}

#define TEST_SAVED_SECONDS 42
#define TEST_RESTORE_TIME ((time_t)100)

static void test_create_game_port_wires_saved_game_functions(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    bool capture_wired = game_port->capture_saved_game_f == capture_saved_game;
    bool restore_wired = game_port->restore_saved_game_f == restore_saved_game;
    destroy_game_port(game_port);
    ASSERT_TRUE(capture_wired);
    ASSERT_TRUE(restore_wired);
}

static void test_game_port_saves_and_restores_a_game_through_the_port(void)
{
    srand(TEST_SEED);
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    NumberGeneration* puzzle =
        game_port->generate_random_numbers_f(game_port->difficulty);
    ASSERT_NOT_NULL(puzzle);
    Board board;
    game_port->init_board_f(&board, puzzle);
    BoardInput input = {.kind = BOARD_INPUT_DIGIT, .digit = TEST_TYPED_VALUE};
    game_port->apply_board_input_f(&board, input);
    SavedGame saved;
    bool captured = game_port->capture_saved_game_f(
        &board, TEST_SAVED_SECONDS, game_port->difficulty, &saved);
    Board restored;
    GameTimer timer;
    NumberGeneration* copy =
        captured ? game_port->restore_saved_game_f(&saved, &restored, &timer,
                                                   TEST_RESTORE_TIME)
                 : NULL;
    bool restored_ok =
        copy != NULL && restored.puzzle == copy && copy != puzzle &&
        memcmp(copy, puzzle, sizeof *puzzle) == 0 &&
        restored.move_count == game_port->board_move_count_f(&board);
    int typed_value = restored_ok ? restored.slots[board.cursor].value : -1;
    int32_t seconds = timer.elapsed_seconds;
    game_port->free_number_generation_f(copy);
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_TRUE(captured);
    ASSERT_TRUE(restored_ok);
    ASSERT_EQ_INT(typed_value, TEST_TYPED_VALUE);
    ASSERT_EQ_INT(seconds, TEST_SAVED_SECONDS);
}

static void test_game_port_restore_through_the_port_rejects_a_bad_save(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    SavedGame saved;
    memset(&saved, 0, sizeof saved);
    saved.difficulty      = -1;
    saved.elapsed_seconds = -1;
    Board board;
    GameTimer timer;
    NumberGeneration* puzzle =
        game_port->restore_saved_game_f(&saved, &board, &timer, 0);
    destroy_game_port(game_port);
    ASSERT_NULL(puzzle);
}

static void test_create_game_port_defaults_to_normal_mode(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    int mode = game_port->mode;
    destroy_game_port(game_port);
    ASSERT_EQ_INT(mode, GAME_MODE_NORMAL);
    ASSERT_EQ_INT(GAME_MODE_NORMAL, 0);
}

static void test_create_game_port_wires_the_daily_generator(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    bool wired =
        game_port->generate_daily_challenge_f == generate_daily_challenge;
    destroy_game_port(game_port);
    ASSERT_TRUE(wired);
}

static void test_prepare_daily_challenge_sets_mode_date_and_difficulty(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    bool prepared     = prepare_daily_challenge(game_port, TEST_DAILY_EPOCH);
    GameMode mode     = game_port->mode;
    CalendarDate date = game_port->daily_date;
    int difficulty    = game_port->difficulty;
    destroy_game_port(game_port);
    ASSERT_TRUE(prepared);
    ASSERT_EQ_INT(mode, GAME_MODE_DAILY_CHALLENGE);
    ASSERT_EQ_INT(date.year, 2026);
    ASSERT_EQ_INT(date.month, 10);
    ASSERT_EQ_INT(date.day, 27);
    ASSERT_EQ_INT(difficulty, DIFFICULTY_EASY);
}

static void test_prepare_daily_challenge_next_day_changes_difficulty(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    bool prepared = prepare_daily_challenge(
        game_port, TEST_DAILY_EPOCH + TEST_SECONDS_PER_DAY);
    int difficulty = game_port->difficulty;
    int day        = game_port->daily_date.day;
    destroy_game_port(game_port);
    ASSERT_TRUE(prepared);
    ASSERT_EQ_INT(day, 28);
    ASSERT_EQ_INT(difficulty, DIFFICULTY_MEDIUM);
}

static void test_prepare_daily_challenge_rejects_minus_one(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    GameMode mode_before     = game_port->mode;
    CalendarDate date_before = game_port->daily_date;
    int difficulty_before    = game_port->difficulty;
    bool prepared            = prepare_daily_challenge(game_port, (time_t)-1);
    bool mode_unchanged      = game_port->mode == mode_before;
    bool date_unchanged = game_port->daily_date.year == date_before.year &&
                          game_port->daily_date.month == date_before.month &&
                          game_port->daily_date.day == date_before.day;
    bool difficulty_unchanged = game_port->difficulty == difficulty_before;
    destroy_game_port(game_port);
    ASSERT_FALSE(prepared);
    ASSERT_TRUE(mode_unchanged);
    ASSERT_TRUE(date_unchanged);
    ASSERT_TRUE(difficulty_unchanged);
}

static void
test_generate_game_puzzle_normal_mode_uses_the_random_generator(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    NumberGeneration* puzzle = generate_game_puzzle(game_port);
    ASSERT_NOT_NULL(puzzle);
    int random_calls   = fake_generate_calls;
    int daily_calls    = fake_generate_daily_calls;
    int generated_with = fake_generate_difficulty;
    int difficulty     = game_port->difficulty;
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_EQ_INT(random_calls, 1);
    ASSERT_EQ_INT(daily_calls, 0);
    ASSERT_EQ_INT(generated_with, difficulty);
}

static void test_generate_game_puzzle_daily_mode_uses_the_daily_generator(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    ASSERT_TRUE(prepare_daily_challenge(game_port, TEST_DAILY_EPOCH));
    NumberGeneration* puzzle = generate_game_puzzle(game_port);
    ASSERT_NOT_NULL(puzzle);
    int random_calls           = fake_generate_calls;
    int daily_calls            = fake_generate_daily_calls;
    CalendarDate generated_for = fake_generate_daily_date;
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_EQ_INT(random_calls, 0);
    ASSERT_EQ_INT(daily_calls, 1);
    ASSERT_EQ_INT(generated_for.year, 2026);
    ASSERT_EQ_INT(generated_for.month, 10);
    ASSERT_EQ_INT(generated_for.day, 27);
}

static void test_restart_game_in_daily_mode_replays_the_same_puzzle(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    ASSERT_TRUE(prepare_daily_challenge(game_port, TEST_DAILY_EPOCH));
    NumberGeneration* puzzle = generate_game_puzzle(game_port);
    ASSERT_NOT_NULL(puzzle);
    Board board;
    game_port->init_board_f(&board, puzzle);
    uintptr_t old_address = (uintptr_t)puzzle;
    bool restarted        = restart_game(game_port, &board, &puzzle);
    bool replaced         = (uintptr_t)puzzle != old_address;
    bool borrowed         = board.puzzle == puzzle;
    bool fresh            = board_is_fresh(&board);
    NumberGeneration reference;
    build_fixture_puzzle(&reference, fixture_old_hidden, TEST_OLD_HIDDEN_COUNT);
    bool same_shape = memcmp(puzzle, &reference, sizeof reference) == 0;
    int daily_calls = fake_generate_daily_calls;
    int free_calls  = fake_free_calls;
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_TRUE(restarted);
    ASSERT_TRUE(replaced);
    ASSERT_TRUE(borrowed);
    ASSERT_TRUE(fresh);
    ASSERT_TRUE(same_shape);
    ASSERT_EQ_INT(daily_calls, 2); /* one to start, one to restart */
    ASSERT_EQ_INT(free_calls, 1);
}

/* DoD: "two separate game sessions on the same date" give the same puzzle,
 * through the real generator, even with the rand() stream seeded differently
 * in between (the daily puzzle must not depend on it). */
static void test_two_ports_prepared_the_same_day_give_the_same_puzzle(void)
{
    GamePort* first = create_game_port();
    ASSERT_NOT_NULL(first);
    ASSERT_TRUE(prepare_daily_challenge(first, TEST_DAILY_EPOCH));
    srand(1);
    NumberGeneration* puzzle_first = generate_game_puzzle(first);
    ASSERT_NOT_NULL(puzzle_first);

    GamePort* second = create_game_port();
    ASSERT_NOT_NULL(second);
    ASSERT_TRUE(prepare_daily_challenge(second, TEST_DAILY_EPOCH +
                                                    TEST_SECONDS_PER_DAY - 1));
    srand(2);
    NumberGeneration* puzzle_second = generate_game_puzzle(second);
    ASSERT_NOT_NULL(puzzle_second);

    bool equal = memcmp(puzzle_first, puzzle_second, sizeof *puzzle_first) == 0;
    first->free_number_generation_f(puzzle_first);
    second->free_number_generation_f(puzzle_second);
    destroy_game_port(first);
    destroy_game_port(second);
    ASSERT_TRUE(equal);
}

static void
test_game_port_records_statistics_true_for_normal_false_for_daily(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    bool normal_records = game_port_records_statistics(game_port);
    ASSERT_TRUE(prepare_daily_challenge(game_port, TEST_DAILY_EPOCH));
    bool daily_records = game_port_records_statistics(game_port);
    destroy_game_port(game_port);
    ASSERT_TRUE(normal_records);
    ASSERT_FALSE(daily_records);
}

static void test_game_port_allows_saving_true_for_normal_false_for_daily(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    bool normal_allows = game_port_allows_saving(game_port);
    ASSERT_TRUE(prepare_daily_challenge(game_port, TEST_DAILY_EPOCH));
    bool daily_allows = game_port_allows_saving(game_port);
    destroy_game_port(game_port);
    ASSERT_TRUE(normal_allows);
    ASSERT_FALSE(daily_allows);
}

static void test_create_game_port_wires_the_hint_functions(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    bool request_wired = game_port->request_board_hint_f == request_board_hint;
    bool counts_wired =
        game_port->board_counts_for_statistics_f == board_counts_for_statistics;
    destroy_game_port(game_port);
    ASSERT_TRUE(request_wired);
    ASSERT_TRUE(counts_wired);
}

static void test_game_port_hint_through_port_marks_the_game(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    NumberGeneration puzzle;
    build_fixture_puzzle(&puzzle, fixture_old_hidden, TEST_OLD_HIDDEN_COUNT);
    Board board;
    game_port->init_board_f(&board, &puzzle);
    bool fresh_counts = game_port->board_counts_for_statistics_f(&board);
    BoardHint hint    = game_port->request_board_hint_f(&board);
    bool marked_count = game_port->board_counts_for_statistics_f(&board);
    destroy_game_port(game_port);
    ASSERT_TRUE(fresh_counts);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_SUGGESTION);
    ASSERT_FALSE(marked_count);
}

static void test_restart_game_clears_the_hint_mark(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    Board board;
    NumberGeneration* puzzle = start_fake_game(game_port, &board);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    game_port->request_board_hint_f(&board);
    bool marked    = !game_port->board_counts_for_statistics_f(&board);
    bool restarted = restart_game(game_port, &board, &puzzle);
    bool counts    = game_port->board_counts_for_statistics_f(&board);
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_TRUE(marked);
    ASSERT_TRUE(restarted);
    ASSERT_TRUE(counts);
}

static void test_restart_game_failure_keeps_the_hint_mark(void)
{
    GamePort* game_port = create_fake_port();
    ASSERT_NOT_NULL(game_port);
    Board board;
    NumberGeneration* puzzle = start_fake_game(game_port, &board);
    if (puzzle == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(puzzle);
    }
    game_port->request_board_hint_f(&board);
    fake_generate_fails = true;
    bool restarted      = restart_game(game_port, &board, &puzzle);
    bool counts         = game_port->board_counts_for_statistics_f(&board);
    game_port->free_number_generation_f(puzzle);
    destroy_game_port(game_port);
    ASSERT_FALSE(restarted);
    ASSERT_FALSE(counts);
}

int main(void)
{
    RUN_TEST(test_create_game_port_returns_non_null);
    RUN_TEST(test_create_game_port_sets_default_difficulty);
    RUN_TEST(test_create_game_port_wires_number_generation_functions);
    RUN_TEST(test_game_port_generates_and_frees_through_port);
    RUN_TEST(test_destroy_game_port_accepts_null);
    RUN_TEST(test_create_game_port_wires_board_functions);
    RUN_TEST(test_game_port_board_session_through_port_is_correct);
    RUN_TEST(test_game_port_board_session_through_port_is_wrong);
    RUN_TEST(test_game_port_groups_typed_through_port_match_solution);
    RUN_TEST(test_game_port_board_session_without_groups_stays_pending);
    RUN_TEST(test_restart_game_replaces_puzzle_and_frees_old_once);
    RUN_TEST(test_restart_game_resets_board_to_initial_state);
    RUN_TEST(test_restart_game_after_correct_result_starts_pending);
    RUN_TEST(test_restart_game_after_wrong_result_starts_pending);
    RUN_TEST(test_restart_game_before_any_answer_starts_pending);
    RUN_TEST(test_restart_game_uses_new_puzzle_numbers);
    RUN_TEST(test_restart_game_generation_failure_keeps_game);
    RUN_TEST(test_restart_game_after_failure_can_restart_again);
    RUN_TEST(test_restart_game_twice_frees_each_old_puzzle_once);
    RUN_TEST(test_restart_game_with_real_port_replaces_puzzle);
    RUN_TEST(test_game_port_difficulty_reaches_generation_per_level);
    RUN_TEST(test_game_port_board_session_is_correct_per_level);
    RUN_TEST(test_restart_game_keeps_selected_difficulty);
    RUN_TEST(test_restart_game_with_real_port_keeps_level);
    RUN_TEST(test_create_game_port_does_not_inherit_previous_difficulty);
    RUN_TEST(test_create_game_port_wires_undo);
    RUN_TEST(test_game_port_undo_through_port_reverts_move);
    RUN_TEST(test_restart_game_empties_move_history);
    RUN_TEST(test_restart_game_failure_keeps_move_history);
    RUN_TEST(test_restart_game_with_real_port_empties_move_history);
    RUN_TEST(test_create_game_port_wires_timer_functions);
    RUN_TEST(test_game_port_timer_through_port_completes_once);
    RUN_TEST(test_create_game_port_wires_pause_and_resume);
    RUN_TEST(test_create_game_port_wires_lock_and_progress);
    RUN_TEST(test_game_port_pause_through_port_freezes_time);
    RUN_TEST(test_game_port_correct_board_through_port_is_locked);
    RUN_TEST(test_restart_game_unlocks_the_board);
    RUN_TEST(test_create_game_port_wires_move_count);
    RUN_TEST(test_game_port_counts_moves_through_port);
    RUN_TEST(test_restart_game_resets_move_count);
    RUN_TEST(test_restart_game_failure_keeps_move_count);
    RUN_TEST(test_restart_game_with_real_port_resets_move_count);
    RUN_TEST(test_create_game_port_wires_saved_game_functions);
    RUN_TEST(test_game_port_saves_and_restores_a_game_through_the_port);
    RUN_TEST(test_game_port_restore_through_the_port_rejects_a_bad_save);
    RUN_TEST(test_create_game_port_defaults_to_normal_mode);
    RUN_TEST(test_create_game_port_wires_the_daily_generator);
    RUN_TEST(test_prepare_daily_challenge_sets_mode_date_and_difficulty);
    RUN_TEST(test_prepare_daily_challenge_next_day_changes_difficulty);
    RUN_TEST(test_prepare_daily_challenge_rejects_minus_one);
    RUN_TEST(test_generate_game_puzzle_normal_mode_uses_the_random_generator);
    RUN_TEST(test_generate_game_puzzle_daily_mode_uses_the_daily_generator);
    RUN_TEST(test_restart_game_in_daily_mode_replays_the_same_puzzle);
    RUN_TEST(test_two_ports_prepared_the_same_day_give_the_same_puzzle);
    RUN_TEST(test_game_port_records_statistics_true_for_normal_false_for_daily);
    RUN_TEST(test_game_port_allows_saving_true_for_normal_false_for_daily);
    RUN_TEST(test_create_game_port_wires_the_hint_functions);
    RUN_TEST(test_game_port_hint_through_port_marks_the_game);
    RUN_TEST(test_restart_game_clears_the_hint_mark);
    RUN_TEST(test_restart_game_failure_keeps_the_hint_mark);
    return test_harness_result();
}
