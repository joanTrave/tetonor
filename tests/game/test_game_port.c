#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../test_harness.h"
#include "fixture_puzzle.h"
#include "../../src/game/game_port.h"

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

static void reset_fakes(void)
{
    fake_generate_calls      = 0;
    fake_generate_difficulty = -1;
    fake_generate_fails      = false;
    fake_hidden              = fixture_old_hidden;
    fake_hidden_count        = TEST_OLD_HIDDEN_COUNT;
    fake_free_calls          = 0;
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

static bool palettes_match(const Palette* first, const Palette* second)
{
    for (int role = 0; role < PALETTE_ROLE_COUNT; role++)
    {
        if (first->foregrounds[role].color != second->foregrounds[role].color ||
            first->foregrounds[role].fallback !=
                second->foregrounds[role].fallback)
        {
            return false;
        }
    }
    return first->background.color == second->background.color &&
           first->background.fallback == second->background.fallback;
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
    return groups_match(first->groups, second->groups) &&
           palettes_match(&first->palette, &second->palette);
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
        game_port->generate_random_numbers_f = &fake_generate;
        game_port->free_number_generation_f  = &fake_free;
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
    board->palette.background.color =
        board->palette.background.color == PALETTE_COLOR_RED
            ? PALETTE_COLOR_BLUE
            : PALETTE_COLOR_RED;
    board->focus = BOARD_FOCUS_RESTART;
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
    return test_harness_result();
}
