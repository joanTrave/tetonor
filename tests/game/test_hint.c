#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../test_harness.h"
#include "fixture_puzzle.h"
#include "fixture_saved_game.h"
#include "../../src/game/answer_check.h"
#include "../../src/game/board.h"
#include "../../src/game/hint.h"

#define TEST_SEED_COUNT 30
#define TEST_LEVEL_COUNT 3
#define TEST_UNTOUCHED_BYTE 0x5a
#define TEST_FOLLOW_LIMIT (2 * TETONOR_CELL_COUNT)
#define TEST_LCG_MULTIPLIER 1103515245u
#define TEST_LCG_INCREMENT 12345u
#define TEST_LCG_SHIFT 16
#define TEST_REVEAL_ODDS 3 /* one box in three is kept as a player entry */
#define TEST_WIDE_SUM_TARGET 5
#define TEST_WIDE_PRODUCT_TARGET 6
#define TEST_FIRST_HIDDEN_SLOT 1  /* first hidden slot of F-S at Easy */
#define TEST_SECOND_HIDDEN_SLOT 5 /* second hidden slot of F-S at Easy */
#define TEST_THIRD_HIDDEN_SLOT 8  /* third hidden slot of F-S at Easy */
#define TEST_OPEN_GROUP_CELL 5    /* target 30: only 5 x 6 fits */
#define TEST_DEAD_END_CELL 2      /* target 7 */

static const int test_levels[TEST_LEVEL_COUNT] = {
    DIFFICULTY_EASY, DIFFICULTY_MEDIUM, DIFFICULTY_HARD};

/* See test_answer_search.c: no answer exists, and only the search can tell. */
static const int wide_strip[TETONOR_CELL_COUNT] = {1, 1, 1, 1, 2, 2, 2, 2,
                                                   2, 3, 3, 3, 5, 5, 5, 5};

static void press(Board* board, BoardInputKind kind)
{
    BoardInput input = {.kind = kind};
    apply_board_input(board, input);
}

static void press_digit(Board* board, int digit)
{
    BoardInput input = {.kind = BOARD_INPUT_DIGIT, .digit = digit};
    apply_board_input(board, input);
}

static void press_operation(Board* board, GroupOperation operation)
{
    BoardInput input = {.kind = BOARD_INPUT_OPERATION, .operation = operation};
    apply_board_input(board, input);
}

static void place_group_focus(Board* board, int cell, GroupField field)
{
    board->focus        = BOARD_FOCUS_GROUP;
    board->group_cursor = cell;
    board->group_field  = field;
}

static void place_slot_focus(Board* board, int slot)
{
    board->focus  = BOARD_FOCUS_SLOT;
    board->cursor = slot;
}

static void make_empty_snapshot(BoardSnapshot* snapshot)
{
    snapshot->result = BOARD_RESULT_PENDING;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        snapshot->slot_values[i] = TETONOR_EMPTY_ENTRY;
        clear_operand_group(&snapshot->groups[i]);
    }
}

static void type_solution_slots(BoardSnapshot* snapshot,
                                const NumberGeneration* puzzle)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (puzzle->available_array[i] == TETONOR_HIDDEN_NUMBER)
        {
            snapshot->slot_values[i] = puzzle->solution_array[i];
        }
    }
}

static void type_solution_groups(BoardSnapshot* snapshot,
                                 const NumberGeneration* puzzle)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        snapshot->groups[i] = puzzle->solution_groups[i];
    }
}

static void start_save_board(NumberGeneration* puzzle, Board* board,
                             int difficulty, const BoardSnapshot* entries)
{
    build_save_fixture_puzzle(puzzle, difficulty);
    restore_board(board, puzzle, entries, 0);
}

static void start_empty_save_board(NumberGeneration* puzzle, Board* board)
{
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    start_save_board(puzzle, board, DIFFICULTY_EASY, &entries);
}

static void take_entries(const Board* board, BoardSnapshot* snapshot)
{
    snapshot->result = BOARD_RESULT_PENDING;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        snapshot->slot_values[i] = board->slots[i].value;
        snapshot->groups[i]      = board->groups[i];
    }
}

/* Types what the hint suggests, as a player would; restore_board recomputes
 * the conflicts. */
static void apply_hint(Board* board, const BoardHint* hint)
{
    BoardSnapshot entries;
    take_entries(board, &entries);
    if (hint->target == HINT_TARGET_SLOT)
    {
        entries.slot_values[hint->index] = hint->value;
    }
    else
    {
        entries.groups[hint->index] = hint->group;
    }
    restore_board(board, board->puzzle, &entries, board->move_count);
}

static bool boards_have_same_history(const Board* first, const Board* second)
{
    if (first->history.count != second->history.count ||
        first->history.newest != second->history.newest)
    {
        return false;
    }
    for (int age = 0; age < first->history.count; age++)
    {
        int at = (first->history.newest - age + TETONOR_UNDO_HISTORY_DEPTH) %
                 TETONOR_UNDO_HISTORY_DEPTH;
        if (!snapshots_equal(&first->history.entries[at],
                             &second->history.entries[at]))
        {
            return false;
        }
    }
    return true;
}

static bool boards_have_same_conflicts(const Board* first, const Board* second)
{
    if (first->conflicts.count != second->conflicts.count)
    {
        return false;
    }
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (first->conflicts.slots[i] != second->conflicts.slots[i])
        {
            return false;
        }
        for (int field = 0; field < TETONOR_GROUP_FIELD_COUNT; field++)
        {
            if (first->conflicts.fields[i][field] !=
                second->conflicts.fields[i][field])
            {
                return false;
            }
        }
    }
    return true;
}

/* Everything but hint_used. */
static bool boards_are_unchanged(const Board* first, const Board* second)
{
    if (first->puzzle != second->puzzle || first->cursor != second->cursor ||
        first->group_cursor != second->group_cursor ||
        first->group_field != second->group_field ||
        first->focus != second->focus || first->result != second->result ||
        first->move_count != second->move_count)
    {
        return false;
    }
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (first->slots[i].editable != second->slots[i].editable ||
            first->slots[i].value != second->slots[i].value ||
            first->slot_usages[i] != second->slot_usages[i] ||
            !operand_groups_equal(&first->groups[i], &second->groups[i]))
        {
            return false;
        }
    }
    return boards_have_same_conflicts(first, second) &&
           boards_have_same_history(first, second);
}

static void expect_hint_changes_nothing_but_the_mark(Board* board,
                                                     HintStatus status)
{
    Board before   = *board;
    BoardHint hint = request_board_hint(board);
    ASSERT_EQ_INT(hint.status, status);
    ASSERT_TRUE(boards_are_unchanged(&before, board));
    ASSERT_EQ_INT(board->hint_used, status == HINT_STATUS_SUGGESTION);
}

static bool group_is_filled_in_place(const OperandGroup* group, int smaller,
                                     GroupOperation operation, int greater)
{
    return group->smaller == smaller && group->operation == operation &&
           group->greater == greater;
}

static unsigned next_random(unsigned* state)
{
    *state = *state * TEST_LCG_MULTIPLIER + TEST_LCG_INCREMENT;
    return *state >> TEST_LCG_SHIFT;
}

static bool is_revealed(unsigned* state)
{
    return next_random(state) % TEST_REVEAL_ODDS == 0;
}

/* The entries of a player who follows the generator's solution for part of the
 * board. */
static void reveal_part_of_the_solution(BoardSnapshot* snapshot,
                                        const NumberGeneration* puzzle,
                                        unsigned seed)
{
    unsigned state = seed;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (puzzle->available_array[i] == TETONOR_HIDDEN_NUMBER &&
            is_revealed(&state))
        {
            snapshot->slot_values[i] = puzzle->solution_array[i];
        }
        const OperandGroup* solution = &puzzle->solution_groups[i];
        OperandGroup* entry          = &snapshot->groups[i];
        if (is_revealed(&state))
        {
            entry->smaller = solution->smaller;
        }
        if (is_revealed(&state))
        {
            entry->operation = solution->operation;
        }
        if (is_revealed(&state))
        {
            entry->greater = solution->greater;
        }
    }
}

static bool suggestion_targets_an_open_box(const Board* board,
                                           const BoardHint* hint)
{
    if (hint->target == HINT_TARGET_SLOT)
    {
        return hint->index >= 0 && hint->index < TETONOR_CELL_COUNT &&
               board->slots[hint->index].editable &&
               board->slots[hint->index].value == TETONOR_EMPTY_ENTRY &&
               hint->value >= TETONOR_MIN_VALUE &&
               hint->value <= TETONOR_MAX_VALUE;
    }
    int target = board->puzzle->sum_matrix[hint->index / TETONOR_GRID_SIZE]
                                          [hint->index % TETONOR_GRID_SIZE];
    return hint->index >= 0 && hint->index < TETONOR_CELL_COUNT &&
           !is_operand_group_filled(&board->groups[hint->index]) &&
           is_operand_group_valid(&hint->group, target);
}

/* Asks, types and asks again until the hint says the board is full. After
 * every typed hint the board must show no conflict. */
static void follow_hints_until_full(Board* board)
{
    int requests = 0;
    for (;;)
    {
        ASSERT_TRUE(requests < TEST_FOLLOW_LIMIT);
        requests++;
        BoardHint hint = request_board_hint(board);
        if (hint.status == HINT_STATUS_BOARD_FULL)
        {
            break;
        }
        ASSERT_EQ_INT(hint.status, HINT_STATUS_SUGGESTION);
        ASSERT_TRUE(suggestion_targets_an_open_box(board, &hint));
        apply_hint(board, &hint);
        ASSERT_EQ_INT(board->conflicts.count, 0);
    }
    int strip[TETONOR_CELL_COUNT];
    build_board_strip(board, strip);
    ASSERT_TRUE(is_answer_correct(board->puzzle, strip, board->groups));
}

static void test_request_board_hint_leaves_board_unchanged_on_a_suggestion(void)
{
    NumberGeneration puzzle;
    Board board;
    start_empty_save_board(&puzzle, &board);
    expect_hint_changes_nothing_but_the_mark(&board, HINT_STATUS_SUGGESTION);
}

static void test_request_board_hint_leaves_a_played_board_unchanged(void)
{
    NumberGeneration puzzle;
    Board board;
    start_empty_save_board(&puzzle, &board);
    press_digit(&board, 1);
    place_group_focus(&board, 0, GROUP_FIELD_SMALLER);
    press_digit(&board, 1);
    place_group_focus(&board, 0, GROUP_FIELD_OPERATION);
    press_operation(&board, GROUP_OPERATION_ADD);
    place_group_focus(&board, TEST_OPEN_GROUP_CELL, GROUP_FIELD_OPERATION);
    press_operation(&board, GROUP_OPERATION_MULTIPLY);
    place_group_focus(&board, 1, GROUP_FIELD_GREATER);
    ASSERT_EQ_INT(board.history.count, 4);
    expect_hint_changes_nothing_but_the_mark(&board, HINT_STATUS_SUGGESTION);
    ASSERT_EQ_INT(board.history.count, 4);
    ASSERT_EQ_INT(board.move_count, 4);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    ASSERT_EQ_INT(board.group_cursor, 1);
    ASSERT_EQ_INT(board.group_field, GROUP_FIELD_GREATER);
}

static void test_request_board_hint_leaves_board_unchanged_with_no_hint(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    build_save_fixture_puzzle(&puzzle, DIFFICULTY_EASY);
    type_solution_slots(&entries, &puzzle);
    type_solution_groups(&entries, &puzzle);
    restore_board(&board, &puzzle, &entries, 0);
    expect_hint_changes_nothing_but_the_mark(&board, HINT_STATUS_BOARD_FULL);

    entries.slot_values[TEST_FIRST_HIDDEN_SLOT] = TETONOR_MAX_VALUE;
    restore_board(&board, &puzzle, &entries, 0);
    ASSERT_TRUE(board.conflicts.count > 0);
    expect_hint_changes_nothing_but_the_mark(&board, HINT_STATUS_CONFLICTS);

    entries.slot_values[TEST_FIRST_HIDDEN_SLOT] =
        puzzle.solution_array[TEST_FIRST_HIDDEN_SLOT];
    restore_board(&board, &puzzle, &entries, 0);
    board.focus = BOARD_FOCUS_SUBMIT;
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_TRUE(is_board_locked(&board));
    expect_hint_changes_nothing_but_the_mark(&board, HINT_STATUS_SOLVED);

    make_empty_snapshot(&entries);
    entries.groups[TEST_DEAD_END_CELL] =
        (OperandGroup){1, GROUP_OPERATION_ADD, 6};
    restore_board(&board, &puzzle, &entries, 0);
    ASSERT_EQ_INT(board.conflicts.count, 0);
    expect_hint_changes_nothing_but_the_mark(&board, HINT_STATUS_NO_SUGGESTION);
}

static void test_request_board_hint_suggests_on_fresh_board(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
        {
            srand(seed);
            NumberGeneration* puzzle =
                generate_random_numbers(test_levels[level]);
            ASSERT_NOT_NULL(puzzle);
            Board board;
            init_board(&board, puzzle);
            BoardHint hint = request_board_hint(&board);
            ASSERT_EQ_INT(hint.status, HINT_STATUS_SUGGESTION);
            ASSERT_TRUE(board.hint_used);
            ASSERT_TRUE(suggestion_targets_an_open_box(&board, &hint));
            free_number_generation(puzzle);
        }
    }
}

static void test_request_board_hint_followed_never_conflicts_and_solves(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
        {
            srand(seed);
            NumberGeneration* puzzle =
                generate_random_numbers(test_levels[level]);
            ASSERT_NOT_NULL(puzzle);
            Board board;
            init_board(&board, puzzle);
            follow_hints_until_full(&board);
            free_number_generation(puzzle);
        }
    }
}

static void test_request_board_hint_followed_after_player_entries_solves(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
        {
            srand(seed);
            NumberGeneration* puzzle =
                generate_random_numbers(test_levels[level]);
            ASSERT_NOT_NULL(puzzle);
            BoardSnapshot entries;
            make_empty_snapshot(&entries);
            reveal_part_of_the_solution(&entries, puzzle, seed);
            Board board;
            restore_board(&board, puzzle, &entries, 0);
            ASSERT_EQ_INT(board.conflicts.count, 0);
            follow_hints_until_full(&board);
            for (int i = 0; i < TETONOR_CELL_COUNT; i++)
            {
                ASSERT_TRUE(entries.slot_values[i] == TETONOR_EMPTY_ENTRY ||
                            board.slots[i].value == entries.slot_values[i]);
                ASSERT_TRUE(entries.groups[i].smaller == TETONOR_EMPTY_ENTRY ||
                            board.groups[i].smaller ==
                                entries.groups[i].smaller);
                ASSERT_TRUE(
                    entries.groups[i].operation == GROUP_OPERATION_NONE ||
                    board.groups[i].operation == entries.groups[i].operation);
                ASSERT_TRUE(entries.groups[i].greater == TETONOR_EMPTY_ENTRY ||
                            board.groups[i].greater ==
                                entries.groups[i].greater);
            }
            free_number_generation(puzzle);
        }
    }
}

static void test_request_board_hint_follows_the_players_own_valid_answer(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    entries.groups[0]  = (OperandGroup){1, GROUP_OPERATION_MULTIPLY, 3};
    entries.groups[13] = (OperandGroup){1, GROUP_OPERATION_ADD, 2};
    start_save_board(&puzzle, &board, DIFFICULTY_EASY, &entries);
    ASSERT_EQ_INT(board.conflicts.count, 0);
    follow_hints_until_full(&board);
    ASSERT_TRUE(group_is_filled_in_place(&board.groups[0], 1,
                                         GROUP_OPERATION_MULTIPLY, 3));
    ASSERT_TRUE(
        group_is_filled_in_place(&board.groups[13], 1, GROUP_OPERATION_ADD, 2));
    ASSERT_TRUE(group_is_filled_in_place(&board.groups[1], 1,
                                         GROUP_OPERATION_MULTIPLY, 2));
}

static void test_request_board_hint_prefers_focused_empty_slot(void)
{
    NumberGeneration puzzle;
    Board board;
    start_empty_save_board(&puzzle, &board);
    place_slot_focus(&board, TEST_THIRD_HIDDEN_SLOT);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_SUGGESTION);
    ASSERT_EQ_INT(hint.target, HINT_TARGET_SLOT);
    ASSERT_EQ_INT(hint.index, TEST_THIRD_HIDDEN_SLOT);
    ASSERT_EQ_INT(hint.value, save_fixture_strip[TEST_THIRD_HIDDEN_SLOT]);
}

static void
test_request_board_hint_ignores_a_focused_group_when_slot_is_open(void)
{
    NumberGeneration puzzle;
    Board board;
    start_empty_save_board(&puzzle, &board);
    place_group_focus(&board, TEST_OPEN_GROUP_CELL, GROUP_FIELD_SMALLER);
    place_slot_focus(&board, TEST_SECOND_HIDDEN_SLOT);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.target, HINT_TARGET_SLOT);
    ASSERT_EQ_INT(hint.index, TEST_SECOND_HIDDEN_SLOT);
    ASSERT_EQ_INT(hint.value, save_fixture_strip[TEST_SECOND_HIDDEN_SLOT]);
}

static void test_request_board_hint_prefers_focused_open_group(void)
{
    NumberGeneration puzzle;
    Board board;
    start_empty_save_board(&puzzle, &board);
    place_group_focus(&board, TEST_OPEN_GROUP_CELL, GROUP_FIELD_OPERATION);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_SUGGESTION);
    ASSERT_EQ_INT(hint.target, HINT_TARGET_GROUP);
    ASSERT_EQ_INT(hint.index, TEST_OPEN_GROUP_CELL);
    ASSERT_TRUE(
        group_is_filled_in_place(&hint.group, 5, GROUP_OPERATION_MULTIPLY, 6));
}

static void test_request_board_hint_focused_group_beats_an_empty_slot(void)
{
    NumberGeneration puzzle;
    Board board;
    start_empty_save_board(&puzzle, &board);
    ASSERT_EQ_INT(board.cursor, TEST_FIRST_HIDDEN_SLOT);
    place_group_focus(&board, TEST_OPEN_GROUP_CELL, GROUP_FIELD_SMALLER);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.target, HINT_TARGET_GROUP);
    ASSERT_EQ_INT(hint.index, TEST_OPEN_GROUP_CELL);
}

static void
test_request_board_hint_falls_to_first_open_group_when_slot_typed(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    entries.slot_values[TEST_THIRD_HIDDEN_SLOT] =
        save_fixture_strip[TEST_THIRD_HIDDEN_SLOT];
    start_save_board(&puzzle, &board, DIFFICULTY_EASY, &entries);
    place_slot_focus(&board, TEST_THIRD_HIDDEN_SLOT);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_SUGGESTION);
    ASSERT_EQ_INT(hint.target, HINT_TARGET_GROUP);
    ASSERT_EQ_INT(hint.index, 0);
}

static void test_request_board_hint_falls_to_first_open_group_when_filled(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    entries.groups[0] = (OperandGroup){1, GROUP_OPERATION_ADD, 2};
    start_save_board(&puzzle, &board, DIFFICULTY_EASY, &entries);
    place_group_focus(&board, 0, GROUP_FIELD_SMALLER);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.target, HINT_TARGET_GROUP);
    ASSERT_EQ_INT(hint.index, 1);
    ASSERT_TRUE(
        group_is_filled_in_place(&hint.group, 1, GROUP_OPERATION_MULTIPLY, 2));
}

static void test_request_board_hint_first_open_group_follows_cell_order(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    build_save_fixture_puzzle(&puzzle, DIFFICULTY_EASY);
    type_solution_slots(&entries, &puzzle);
    for (int cell = 0; cell < 3; cell++)
    {
        entries.groups[cell] = puzzle.solution_groups[cell];
    }
    restore_board(&board, &puzzle, &entries, 0);
    board.focus    = BOARD_FOCUS_SUBMIT;
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_SUGGESTION);
    ASSERT_EQ_INT(hint.target, HINT_TARGET_GROUP);
    ASSERT_EQ_INT(hint.index, 3);
    ASSERT_TRUE(
        group_is_filled_in_place(&hint.group, 3, GROUP_OPERATION_MULTIPLY, 4));
}

static void test_request_board_hint_first_open_group_skips_partial_groups(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    entries.groups[0].operation = GROUP_OPERATION_ADD;
    start_save_board(&puzzle, &board, DIFFICULTY_EASY, &entries);
    board.focus    = BOARD_FOCUS_RESTART;
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.target, HINT_TARGET_GROUP);
    ASSERT_EQ_INT(hint.index, 0);
    ASSERT_EQ_INT(hint.group.operation, GROUP_OPERATION_ADD);
}

static void test_request_board_hint_first_empty_slot_when_groups_are_full(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    build_save_fixture_puzzle(&puzzle, DIFFICULTY_EASY);
    type_solution_groups(&entries, &puzzle);
    restore_board(&board, &puzzle, &entries, 0);
    board.focus    = BOARD_FOCUS_SUBMIT;
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_SUGGESTION);
    ASSERT_EQ_INT(hint.target, HINT_TARGET_SLOT);
    ASSERT_EQ_INT(hint.index, TEST_FIRST_HIDDEN_SLOT);
    ASSERT_EQ_INT(hint.value, save_fixture_strip[TEST_FIRST_HIDDEN_SLOT]);
}

static void test_request_board_hint_first_empty_slot_skips_typed_slots(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    build_save_fixture_puzzle(&puzzle, DIFFICULTY_EASY);
    type_solution_groups(&entries, &puzzle);
    entries.slot_values[TEST_FIRST_HIDDEN_SLOT] =
        save_fixture_strip[TEST_FIRST_HIDDEN_SLOT];
    restore_board(&board, &puzzle, &entries, 0);
    place_slot_focus(&board, TEST_FIRST_HIDDEN_SLOT);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.target, HINT_TARGET_SLOT);
    ASSERT_EQ_INT(hint.index, TEST_SECOND_HIDDEN_SLOT);
    ASSERT_EQ_INT(hint.value, save_fixture_strip[TEST_SECOND_HIDDEN_SLOT]);
}

static void test_request_board_hint_keeps_partial_group_boxes(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    entries.groups[TEST_OPEN_GROUP_CELL].operation = GROUP_OPERATION_MULTIPLY;
    entries.groups[3].greater                      = 4;
    entries.groups[12].smaller                     = 1;
    start_save_board(&puzzle, &board, DIFFICULTY_EASY, &entries);
    place_group_focus(&board, TEST_OPEN_GROUP_CELL, GROUP_FIELD_SMALLER);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.index, TEST_OPEN_GROUP_CELL);
    ASSERT_TRUE(
        group_is_filled_in_place(&hint.group, 5, GROUP_OPERATION_MULTIPLY, 6));
    place_group_focus(&board, 3, GROUP_FIELD_SMALLER);
    hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.index, 3);
    ASSERT_TRUE(
        group_is_filled_in_place(&hint.group, 3, GROUP_OPERATION_MULTIPLY, 4));
    place_group_focus(&board, 12, GROUP_FIELD_GREATER);
    hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.index, 12);
    ASSERT_TRUE(
        group_is_filled_in_place(&hint.group, 1, GROUP_OPERATION_ADD, 3));
}

static void test_request_board_hint_keeps_the_typed_strip(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    entries.slot_values[TEST_FIRST_HIDDEN_SLOT]  = 1;
    entries.slot_values[TEST_SECOND_HIDDEN_SLOT] = 3;
    start_save_board(&puzzle, &board, DIFFICULTY_EASY, &entries);
    place_slot_focus(&board, TEST_THIRD_HIDDEN_SLOT);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.target, HINT_TARGET_SLOT);
    ASSERT_EQ_INT(hint.index, TEST_THIRD_HIDDEN_SLOT);
    ASSERT_EQ_INT(hint.value, save_fixture_strip[TEST_THIRD_HIDDEN_SLOT]);
}

static void test_request_board_hint_solved_board(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    build_save_fixture_puzzle(&puzzle, DIFFICULTY_EASY);
    type_solution_slots(&entries, &puzzle);
    type_solution_groups(&entries, &puzzle);
    restore_board(&board, &puzzle, &entries, 0);
    board.focus = BOARD_FOCUS_SUBMIT;
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_SOLVED);
    ASSERT_FALSE(board.hint_used);
}

static void test_request_board_hint_conflicts(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    entries.slot_values[TEST_FIRST_HIDDEN_SLOT] = TETONOR_MAX_VALUE;
    start_save_board(&puzzle, &board, DIFFICULTY_EASY, &entries);
    ASSERT_TRUE(board.conflicts.count > 0);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_CONFLICTS);
    ASSERT_FALSE(board.hint_used);
}

static void test_request_board_hint_conflict_in_a_group(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    entries.groups[0] = (OperandGroup){2, GROUP_OPERATION_ADD, 1};
    start_save_board(&puzzle, &board, DIFFICULTY_EASY, &entries);
    ASSERT_TRUE(board.conflicts.count > 0);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_CONFLICTS);
    ASSERT_FALSE(board.hint_used);
}

static void test_request_board_hint_conflicts_come_before_a_full_board(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    build_save_fixture_puzzle(&puzzle, DIFFICULTY_EASY);
    type_solution_slots(&entries, &puzzle);
    type_solution_groups(&entries, &puzzle);
    entries.slot_values[TEST_FIRST_HIDDEN_SLOT] = TETONOR_MAX_VALUE;
    restore_board(&board, &puzzle, &entries, 0);
    ASSERT_TRUE(board.conflicts.count > 0);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_CONFLICTS);
}

static void test_request_board_hint_full_board_that_is_correct(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    build_save_fixture_puzzle(&puzzle, DIFFICULTY_EASY);
    type_solution_slots(&entries, &puzzle);
    type_solution_groups(&entries, &puzzle);
    restore_board(&board, &puzzle, &entries, 0);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_BOARD_FULL);
    ASSERT_FALSE(board.hint_used);
}

static void test_request_board_hint_full_board_that_is_wrong(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    build_save_fixture_puzzle(&puzzle, DIFFICULTY_EASY);
    type_solution_slots(&entries, &puzzle);
    type_solution_groups(&entries, &puzzle);
    entries.slot_values[TEST_FIRST_HIDDEN_SLOT] = 2;
    restore_board(&board, &puzzle, &entries, 0);
    board.focus = BOARD_FOCUS_SUBMIT;
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    ASSERT_EQ_INT(board.conflicts.count, 0);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_BOARD_FULL);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    ASSERT_FALSE(board.hint_used);
}

static void test_request_board_hint_dead_end(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    entries.groups[TEST_DEAD_END_CELL] =
        (OperandGroup){1, GROUP_OPERATION_ADD, 6};
    start_save_board(&puzzle, &board, DIFFICULTY_EASY, &entries);
    ASSERT_EQ_INT(board.conflicts.count, 0);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_NO_SUGGESTION);
    ASSERT_FALSE(board.hint_used);
}

static void test_request_board_hint_dead_end_from_a_wrong_slot(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    entries.slot_values[TEST_FIRST_HIDDEN_SLOT] = 2;
    start_save_board(&puzzle, &board, DIFFICULTY_EASY, &entries);
    ASSERT_EQ_INT(board.conflicts.count, 0);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_NO_SUGGESTION);
    ASSERT_FALSE(board.hint_used);
}

static void test_request_board_hint_gives_up_on_an_unsolvable_wide_search(void)
{
    NumberGeneration puzzle;
    memset(&puzzle, 0, sizeof puzzle);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        int target = i < TETONOR_PAIR_COUNT ? TEST_WIDE_SUM_TARGET
                                            : TEST_WIDE_PRODUCT_TARGET;
        puzzle.sum_matrix[i / TETONOR_GRID_SIZE][i % TETONOR_GRID_SIZE] =
            target;
        puzzle.available_array[i] = wide_strip[i];
    }
    Board board;
    init_board(&board, &puzzle);
    ASSERT_EQ_INT(board.conflicts.count, 0);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_NO_SUGGESTION);
    ASSERT_FALSE(board.hint_used);
}

static void test_request_board_hint_without_editable_slots_hints_groups(void)
{
    NumberGeneration puzzle;
    Board board;
    build_fixture_puzzle(&puzzle, NULL, 0);
    init_board(&board, &puzzle);
    ASSERT_EQ_INT(board.cursor, TETONOR_NO_CURSOR);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_SUGGESTION);
    ASSERT_EQ_INT(hint.target, HINT_TARGET_GROUP);
    ASSERT_EQ_INT(hint.index, 0);
    ASSERT_TRUE(suggestion_targets_an_open_box(&board, &hint));
    follow_hints_until_full(&board);
}

static void test_request_board_hint_no_hint_replies_are_zeroed(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    build_save_fixture_puzzle(&puzzle, DIFFICULTY_EASY);
    type_solution_slots(&entries, &puzzle);
    type_solution_groups(&entries, &puzzle);
    restore_board(&board, &puzzle, &entries, 0);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_BOARD_FULL);
    ASSERT_EQ_INT(hint.index, 0);
    ASSERT_EQ_INT(hint.value, 0);
}

static void test_request_board_hint_repeated_request_is_stable(void)
{
    NumberGeneration puzzle;
    Board board;
    start_empty_save_board(&puzzle, &board);
    BoardHint first  = request_board_hint(&board);
    BoardHint second = request_board_hint(&board);
    ASSERT_EQ_INT(first.status, second.status);
    ASSERT_EQ_INT(first.target, second.target);
    ASSERT_EQ_INT(first.index, second.index);
    ASSERT_EQ_INT(first.value, second.value);
    ASSERT_TRUE(group_is_filled_in_place(&first.group, second.group.smaller,
                                         second.group.operation,
                                         second.group.greater));
}

static void test_request_board_hint_does_not_read_solution_fields(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
        {
            srand(seed);
            NumberGeneration* puzzle =
                generate_random_numbers(test_levels[level]);
            ASSERT_NOT_NULL(puzzle);
            memset(puzzle->solution_array, TEST_UNTOUCHED_BYTE,
                   sizeof puzzle->solution_array);
            memset(puzzle->solution_groups, TEST_UNTOUCHED_BYTE,
                   sizeof puzzle->solution_groups);
            Board board;
            init_board(&board, puzzle);
            follow_hints_until_full(&board);
            free_number_generation(puzzle);
        }
    }
}

static void test_board_counts_for_statistics_fresh_board(void)
{
    NumberGeneration puzzle;
    Board board;
    start_empty_save_board(&puzzle, &board);
    ASSERT_TRUE(board_counts_for_statistics(&board));
}

static void test_board_counts_for_statistics_false_after_a_suggestion(void)
{
    NumberGeneration puzzle;
    Board board;
    start_empty_save_board(&puzzle, &board);
    BoardHint hint = request_board_hint(&board);
    ASSERT_EQ_INT(hint.status, HINT_STATUS_SUGGESTION);
    ASSERT_FALSE(board_counts_for_statistics(&board));
}

static void test_board_counts_for_statistics_true_after_no_hint_replies(void)
{
    NumberGeneration puzzle;
    Board board;
    BoardSnapshot entries;
    make_empty_snapshot(&entries);
    entries.groups[TEST_DEAD_END_CELL] =
        (OperandGroup){1, GROUP_OPERATION_ADD, 6};
    start_save_board(&puzzle, &board, DIFFICULTY_EASY, &entries);
    ASSERT_EQ_INT(request_board_hint(&board).status, HINT_STATUS_NO_SUGGESTION);
    ASSERT_TRUE(board_counts_for_statistics(&board));
    entries.slot_values[TEST_FIRST_HIDDEN_SLOT] = TETONOR_MAX_VALUE;
    restore_board(&board, &puzzle, &entries, 0);
    ASSERT_EQ_INT(request_board_hint(&board).status, HINT_STATUS_CONFLICTS);
    ASSERT_TRUE(board_counts_for_statistics(&board));
    make_empty_snapshot(&entries);
    type_solution_slots(&entries, &puzzle);
    type_solution_groups(&entries, &puzzle);
    restore_board(&board, &puzzle, &entries, 0);
    ASSERT_EQ_INT(request_board_hint(&board).status, HINT_STATUS_BOARD_FULL);
    ASSERT_TRUE(board_counts_for_statistics(&board));
    board.focus = BOARD_FOCUS_SUBMIT;
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(request_board_hint(&board).status, HINT_STATUS_SOLVED);
    ASSERT_TRUE(board_counts_for_statistics(&board));
}

static void test_board_counts_for_statistics_stays_false_after_undo(void)
{
    NumberGeneration puzzle;
    Board board;
    start_empty_save_board(&puzzle, &board);
    press_digit(&board, 1);
    ASSERT_EQ_INT(request_board_hint(&board).status, HINT_STATUS_SUGGESTION);
    press_digit(&board, 2);
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_FALSE(board_counts_for_statistics(&board));
}

static void test_board_counts_for_statistics_stays_false_after_moves(void)
{
    NumberGeneration puzzle;
    Board board;
    start_empty_save_board(&puzzle, &board);
    ASSERT_EQ_INT(request_board_hint(&board).status, HINT_STATUS_SUGGESTION);
    press_digit(&board, 1);
    press(&board, BOARD_INPUT_DELETE);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    ASSERT_FALSE(board_counts_for_statistics(&board));
}

static void test_board_counts_for_statistics_true_again_after_init_board(void)
{
    NumberGeneration puzzle;
    Board board;
    start_empty_save_board(&puzzle, &board);
    ASSERT_EQ_INT(request_board_hint(&board).status, HINT_STATUS_SUGGESTION);
    ASSERT_FALSE(board_counts_for_statistics(&board));
    init_board(&board, &puzzle);
    ASSERT_TRUE(board_counts_for_statistics(&board));
}

int main(void)
{
    RUN_TEST(test_request_board_hint_leaves_board_unchanged_on_a_suggestion);
    RUN_TEST(test_request_board_hint_leaves_a_played_board_unchanged);
    RUN_TEST(test_request_board_hint_leaves_board_unchanged_with_no_hint);
    RUN_TEST(test_request_board_hint_suggests_on_fresh_board);
    RUN_TEST(test_request_board_hint_followed_never_conflicts_and_solves);
    RUN_TEST(test_request_board_hint_followed_after_player_entries_solves);
    RUN_TEST(test_request_board_hint_follows_the_players_own_valid_answer);
    RUN_TEST(test_request_board_hint_prefers_focused_empty_slot);
    RUN_TEST(test_request_board_hint_ignores_a_focused_group_when_slot_is_open);
    RUN_TEST(test_request_board_hint_prefers_focused_open_group);
    RUN_TEST(test_request_board_hint_focused_group_beats_an_empty_slot);
    RUN_TEST(test_request_board_hint_falls_to_first_open_group_when_slot_typed);
    RUN_TEST(test_request_board_hint_falls_to_first_open_group_when_filled);
    RUN_TEST(test_request_board_hint_first_open_group_follows_cell_order);
    RUN_TEST(test_request_board_hint_first_open_group_skips_partial_groups);
    RUN_TEST(test_request_board_hint_first_empty_slot_when_groups_are_full);
    RUN_TEST(test_request_board_hint_first_empty_slot_skips_typed_slots);
    RUN_TEST(test_request_board_hint_keeps_partial_group_boxes);
    RUN_TEST(test_request_board_hint_keeps_the_typed_strip);
    RUN_TEST(test_request_board_hint_solved_board);
    RUN_TEST(test_request_board_hint_conflicts);
    RUN_TEST(test_request_board_hint_conflict_in_a_group);
    RUN_TEST(test_request_board_hint_conflicts_come_before_a_full_board);
    RUN_TEST(test_request_board_hint_full_board_that_is_correct);
    RUN_TEST(test_request_board_hint_full_board_that_is_wrong);
    RUN_TEST(test_request_board_hint_dead_end);
    RUN_TEST(test_request_board_hint_dead_end_from_a_wrong_slot);
    RUN_TEST(test_request_board_hint_gives_up_on_an_unsolvable_wide_search);
    RUN_TEST(test_request_board_hint_without_editable_slots_hints_groups);
    RUN_TEST(test_request_board_hint_no_hint_replies_are_zeroed);
    RUN_TEST(test_request_board_hint_repeated_request_is_stable);
    RUN_TEST(test_request_board_hint_does_not_read_solution_fields);
    RUN_TEST(test_board_counts_for_statistics_fresh_board);
    RUN_TEST(test_board_counts_for_statistics_false_after_a_suggestion);
    RUN_TEST(test_board_counts_for_statistics_true_after_no_hint_replies);
    RUN_TEST(test_board_counts_for_statistics_stays_false_after_undo);
    RUN_TEST(test_board_counts_for_statistics_stays_false_after_moves);
    RUN_TEST(test_board_counts_for_statistics_true_again_after_init_board);
    return test_harness_result();
}
