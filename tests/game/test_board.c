#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "../test_harness.h"
#include "fixture_puzzle.h"
#include "../../src/game/board.h"

#define TEST_FIRST_SLOT 2
#define TEST_SECOND_SLOT 5
#define TEST_THIRD_SLOT 6
#define TEST_LAST_SLOT 15
#define TEST_EDITABLE_COUNT 4
#define TEST_OUT_OF_RANGE_DIGIT TETONOR_DIGIT_BASE
#define TEST_WRONG_VALUE 28
#define TEST_FIELD_COUNT 3
#define TEST_NUMBER_FIELD_COUNT 2
#define TEST_GRID_LAST (TETONOR_GRID_SIZE - 1)
#define TEST_LAST_CELL (TETONOR_CELL_COUNT - 1)
#define TEST_MOVE_KIND_COUNT 4
#define TEST_MOVE_SEQUENCE_LENGTH 5
#define TEST_MOVE_SEQUENCE_COUNT 1024 /* TEST_MOVE_KIND_COUNT ^ LENGTH */

static const int fixture_hidden[TEST_EDITABLE_COUNT] = {
    TEST_FIRST_SLOT, TEST_SECOND_SLOT, TEST_THIRD_SLOT, TEST_LAST_SLOT};
static const int fixture_hidden_values[TEST_EDITABLE_COUNT]     = {2, 5, 6, 29};
static const int fixture_wrong_last_values[TEST_EDITABLE_COUNT] = {
    2, 5, 6, TEST_WRONG_VALUE};
static const GroupField fixture_fields[TEST_FIELD_COUNT] = {
    GROUP_FIELD_SMALLER, GROUP_FIELD_OPERATION, GROUP_FIELD_GREATER};
static const GroupField fixture_number_fields[TEST_NUMBER_FIELD_COUNT] = {
    GROUP_FIELD_SMALLER, GROUP_FIELD_GREATER};
static const BoardInputKind fixture_move_kinds[TEST_MOVE_KIND_COUNT] = {
    BOARD_INPUT_MOVE_LEFT, BOARD_INPUT_MOVE_RIGHT, BOARD_INPUT_MOVE_UP,
    BOARD_INPUT_MOVE_DOWN};

static void start_default_board(NumberGeneration* puzzle, Board* board)
{
    build_fixture_puzzle(puzzle, fixture_hidden, TEST_EDITABLE_COUNT);
    init_board(board, puzzle);
}

static void start_board_without_slots(NumberGeneration* puzzle, Board* board)
{
    build_fixture_puzzle(puzzle, NULL, 0);
    init_board(board, puzzle);
}

static void press(Board* board, BoardInputKind kind)
{
    BoardInput input = {.kind = kind};
    apply_board_input(board, input);
}

static BoardEvent apply_for_event(Board* board, BoardInput input)
{
    return apply_board_input(board, input);
}

static BoardEvent press_for_event(Board* board, BoardInputKind kind)
{
    BoardInput input = {.kind = kind};
    return apply_for_event(board, input);
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

static void press_times(Board* board, BoardInputKind kind, int count)
{
    for (int i = 0; i < count; i++)
    {
        press(board, kind);
    }
}

static void type_value(Board* board, int value)
{
    if (value >= TETONOR_DIGIT_BASE)
    {
        press_digit(board, value / TETONOR_DIGIT_BASE);
    }
    press_digit(board, value % TETONOR_DIGIT_BASE);
}

/* White-box: the solution groups are set directly so that tests of the slots
 * and of the judging do not depend on the group-entry rules. */
static void fill_solution_groups(Board* board)
{
    memcpy(board->groups, fixture_groups, sizeof board->groups);
}

static void place_group_focus(Board* board, int cell, GroupField field)
{
    board->focus        = BOARD_FOCUS_GROUP;
    board->group_cursor = cell;
    board->group_field  = field;
}

/* Starts on the first editable slot; the final RIGHT leaves the focus on the
 * submit control with the cursor on the last slot. */
static void fill_slots(Board* board, const int values[TEST_EDITABLE_COUNT])
{
    for (int i = 0; i < TEST_EDITABLE_COUNT; i++)
    {
        type_value(board, values[i]);
        press(board, BOARD_INPUT_MOVE_RIGHT);
    }
}

static void fill_and_submit(Board* board, const int values[TEST_EDITABLE_COUNT])
{
    fill_solution_groups(board);
    fill_slots(board, values);
    press(board, BOARD_INPUT_ACTIVATE);
}

/* Two DELETEs empty any value of at most two digits. */
static void retype_slot(Board* board, int value)
{
    press(board, BOARD_INPUT_DELETE);
    press(board, BOARD_INPUT_DELETE);
    type_value(board, value);
}

static void start_correct_board(NumberGeneration* puzzle, Board* board)
{
    start_default_board(puzzle, board);
    fill_and_submit(board, fixture_hidden_values);
}

static void start_wrong_board(NumberGeneration* puzzle, Board* board)
{
    start_default_board(puzzle, board);
    fill_and_submit(board, fixture_wrong_last_values);
}

static void empty_group_field(Board* board, int cell, GroupField field)
{
    OperandGroup* group = &board->groups[cell];
    if (field == GROUP_FIELD_SMALLER)
    {
        group->smaller = TETONOR_EMPTY_ENTRY;
    }
    else if (field == GROUP_FIELD_GREATER)
    {
        group->greater = TETONOR_EMPTY_ENTRY;
    }
    else
    {
        group->operation = GROUP_OPERATION_NONE;
    }
}

static int group_number(const Board* board, int cell, GroupField field)
{
    return field == GROUP_FIELD_SMALLER ? board->groups[cell].smaller
                                        : board->groups[cell].greater;
}

static bool groups_equal(const OperandGroup* first, const OperandGroup* second)
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

static bool all_groups_empty(const Board* board)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        const OperandGroup* group = &board->groups[i];
        if (group->smaller != TETONOR_EMPTY_ENTRY ||
            group->operation != GROUP_OPERATION_NONE ||
            group->greater != TETONOR_EMPTY_ENTRY)
        {
            return false;
        }
    }
    return true;
}

static bool slots_equal(const Board* first, const Board* second)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        const BoardSlot* a = &first->slots[i];
        const BoardSlot* b = &second->slots[i];
        if (a->editable != b->editable || (a->editable && a->value != b->value))
        {
            return false;
        }
    }
    return true;
}

static bool boards_equal(const Board* first, const Board* second)
{
    if (first->cursor != second->cursor || first->focus != second->focus ||
        first->result != second->result ||
        first->group_cursor != second->group_cursor ||
        first->group_field != second->group_field)
    {
        return false;
    }
    return slots_equal(first, second) &&
           groups_equal(first->groups, second->groups) &&
           memcmp(first->slot_usages, second->slot_usages,
                  sizeof first->slot_usages) == 0;
}

static bool equal_apart_from_focus(const Board* first, const Board* second)
{
    Board aligned = *first;
    aligned.focus = second->focus;
    return boards_equal(&aligned, second);
}

/* True when the only non-empty group box is the one given. */
static bool only_group_number_is_set(const Board* board, int cell,
                                     GroupField field, int value)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        const OperandGroup* group = &board->groups[i];
        int expected_smaller      = i == cell && field == GROUP_FIELD_SMALLER
                                        ? value
                                        : TETONOR_EMPTY_ENTRY;
        int expected_greater      = i == cell && field == GROUP_FIELD_GREATER
                                        ? value
                                        : TETONOR_EMPTY_ENTRY;
        if (group->smaller != expected_smaller ||
            group->greater != expected_greater ||
            group->operation != GROUP_OPERATION_NONE)
        {
            return false;
        }
    }
    return true;
}

/* Presses the sequence-th combination of TEST_MOVE_SEQUENCE_LENGTH movement
 * inputs, reading sequence as a base-4 number. */
static void press_movement_sequence(Board* board, int sequence)
{
    for (int i = 0; i < TEST_MOVE_SEQUENCE_LENGTH; i++)
    {
        press(board, fixture_move_kinds[sequence % TEST_MOVE_KIND_COUNT]);
        sequence /= TEST_MOVE_KIND_COUNT;
    }
}

static void test_init_board_places_cursor_on_first_editable_slot(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    ASSERT_EQ_INT(board.cursor, TEST_FIRST_SLOT);
}

static void test_init_board_marks_hidden_numbers_as_editable(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        bool expected = puzzle.available_array[i] == TETONOR_HIDDEN_NUMBER;
        ASSERT_EQ_INT(board.slots[i].editable, expected);
    }
}

static void test_init_board_slots_start_empty_with_slot_focus(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (board.slots[i].editable)
        {
            ASSERT_EQ_INT(board.slots[i].value, TETONOR_EMPTY_ENTRY);
        }
    }
}

static void test_init_board_focus_starts_on_slots(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    board.focus = BOARD_FOCUS_SUBMIT;
    init_board(&board, &puzzle);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
}

static void test_init_board_result_is_pending(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void test_init_board_borrows_puzzle(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    ASSERT_TRUE(board.puzzle == &puzzle);
}

static void test_init_board_groups_start_empty(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    ASSERT_TRUE(all_groups_empty(&board));
}

static void test_init_board_group_focus_starts_on_first_smaller_box(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    ASSERT_EQ_INT(board.group_cursor, 0);
    ASSERT_EQ_INT(board.group_field, GROUP_FIELD_SMALLER);
}

static void test_init_board_discards_previous_session(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_and_submit(&board, fixture_hidden_values);
    place_group_focus(&board, TEST_LAST_CELL, GROUP_FIELD_GREATER);
    init_board(&board, &puzzle);
    ASSERT_TRUE(all_groups_empty(&board));
    ASSERT_EQ_INT(board.group_cursor, 0);
    ASSERT_EQ_INT(board.group_field, GROUP_FIELD_SMALLER);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TETONOR_EMPTY_ENTRY);
}

static void test_init_board_without_editable_slots_has_no_cursor(void)
{
    NumberGeneration puzzle;
    Board board;
    start_board_without_slots(&puzzle, &board);
    ASSERT_EQ_INT(board.cursor, TETONOR_NO_CURSOR);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        ASSERT_FALSE(board.slots[i].editable);
    }
}

static void test_apply_board_input_without_editable_slots_starts_on_groups(void)
{
    NumberGeneration puzzle;
    Board board;
    start_board_without_slots(&puzzle, &board);
    press_digit(&board, 5);
    ASSERT_EQ_INT(board.groups[0].smaller, 5);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    ASSERT_EQ_INT(board.cursor, TETONOR_NO_CURSOR);
}

static void
test_apply_board_input_without_editable_slots_never_focuses_slot(void)
{
    NumberGeneration puzzle;
    Board start;
    start_board_without_slots(&puzzle, &start);
    for (int sequence = 0; sequence < TEST_MOVE_SEQUENCE_COUNT; sequence++)
    {
        Board board = start;
        for (int i = 0; i < TEST_MOVE_SEQUENCE_LENGTH; i++)
        {
            press(&board, fixture_move_kinds[(sequence >> (2 * i)) %
                                             TEST_MOVE_KIND_COUNT]);
            ASSERT_TRUE(board.focus != BOARD_FOCUS_SLOT);
            ASSERT_EQ_INT(board.cursor, TETONOR_NO_CURSOR);
        }
    }
}

static void
test_apply_board_input_without_editable_slots_down_reaches_submit(void)
{
    NumberGeneration puzzle;
    Board board;
    start_board_without_slots(&puzzle, &board);
    press_times(&board, BOARD_INPUT_MOVE_DOWN, TEST_GRID_LAST);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    ASSERT_EQ_INT(board.group_cursor, TEST_GRID_LAST * TETONOR_GRID_SIZE);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
    ASSERT_EQ_INT(board.group_cursor, TEST_GRID_LAST * TETONOR_GRID_SIZE);
    ASSERT_EQ_INT(board.cursor, TETONOR_NO_CURSOR);
}

static void test_apply_board_input_without_editable_slots_submit_goes_back(void)
{
    NumberGeneration puzzle;
    Board board;
    start_board_without_slots(&puzzle, &board);
    place_group_focus(&board, TEST_LAST_CELL - 1, GROUP_FIELD_OPERATION);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
    press(&board, BOARD_INPUT_MOVE_UP);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    ASSERT_EQ_INT(board.group_cursor, TEST_LAST_CELL - 1);
    ASSERT_EQ_INT(board.group_field, GROUP_FIELD_OPERATION);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    press(&board, BOARD_INPUT_MOVE_LEFT);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    ASSERT_EQ_INT(board.group_cursor, TEST_LAST_CELL - 1);
    ASSERT_EQ_INT(board.group_field, GROUP_FIELD_OPERATION);
}

static void test_apply_board_input_without_editable_slots_can_be_judged(void)
{
    NumberGeneration puzzle;
    Board board;
    start_board_without_slots(&puzzle, &board);
    fill_solution_groups(&board);
    press_times(&board, BOARD_INPUT_MOVE_DOWN, TETONOR_GRID_SIZE);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void
test_apply_board_input_without_editable_slots_wrong_group_is_wrong(void)
{
    NumberGeneration puzzle;
    Board board;
    start_board_without_slots(&puzzle, &board);
    fill_solution_groups(&board);
    board.groups[0].operation = GROUP_OPERATION_MULTIPLY;
    press_times(&board, BOARD_INPUT_MOVE_DOWN, TETONOR_GRID_SIZE);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
}

static void test_apply_board_input_move_right_visits_editable_slots_only(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    ASSERT_EQ_INT(board.cursor, TEST_SECOND_SLOT);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    ASSERT_EQ_INT(board.cursor, TEST_THIRD_SLOT);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    ASSERT_EQ_INT(board.cursor, TEST_LAST_SLOT);
}

static void
test_apply_board_input_move_right_past_last_slot_focuses_submit(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    for (int i = 0; i < TEST_EDITABLE_COUNT - 1; i++)
    {
        press(&board, BOARD_INPUT_MOVE_RIGHT);
    }
    ASSERT_EQ_INT(board.cursor, TEST_LAST_SLOT);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    ASSERT_EQ_INT(board.cursor, TEST_LAST_SLOT);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    ASSERT_EQ_INT(board.cursor, TEST_LAST_SLOT);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
}

static void test_apply_board_input_move_left_visits_editable_slots_only(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    for (int i = 0; i < TEST_EDITABLE_COUNT - 1; i++)
    {
        press(&board, BOARD_INPUT_MOVE_RIGHT);
    }
    press(&board, BOARD_INPUT_MOVE_LEFT);
    ASSERT_EQ_INT(board.cursor, TEST_THIRD_SLOT);
    press(&board, BOARD_INPUT_MOVE_LEFT);
    ASSERT_EQ_INT(board.cursor, TEST_SECOND_SLOT);
    press(&board, BOARD_INPUT_MOVE_LEFT);
    ASSERT_EQ_INT(board.cursor, TEST_FIRST_SLOT);
}

static void test_apply_board_input_move_left_at_first_slot_stays(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_LEFT);
    press(&board, BOARD_INPUT_MOVE_LEFT);
    ASSERT_EQ_INT(board.cursor, TEST_FIRST_SLOT);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
}

static void test_apply_board_input_move_with_single_editable_slot_stays(void)
{
    NumberGeneration puzzle;
    Board board;
    const int hidden[] = {TEST_THIRD_SLOT};
    build_fixture_puzzle(&puzzle, hidden, 1);
    init_board(&board, &puzzle);
    ASSERT_EQ_INT(board.cursor, TEST_THIRD_SLOT);
    press(&board, BOARD_INPUT_MOVE_LEFT);
    ASSERT_EQ_INT(board.cursor, TEST_THIRD_SLOT);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    ASSERT_EQ_INT(board.cursor, TEST_THIRD_SLOT);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
}

static void test_apply_board_input_move_keeps_typed_values(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 7);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    press(&board, BOARD_INPUT_MOVE_LEFT);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 7);
    ASSERT_EQ_INT(board.slots[TEST_SECOND_SLOT].value, TETONOR_EMPTY_ENTRY);
}

static void test_apply_board_input_move_down_focuses_submit_keeping_cursor(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 7);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    Board before = board;
    press(&board, BOARD_INPUT_MOVE_DOWN);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
    ASSERT_EQ_INT(board.cursor, TEST_SECOND_SLOT);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_TRUE(slots_equal(&board, &before));
}

static void test_apply_board_input_move_up_on_slot_focuses_group(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 7);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    Board before = board;
    press(&board, BOARD_INPUT_MOVE_UP);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    ASSERT_EQ_INT(board.group_cursor, 0);
    ASSERT_EQ_INT(board.group_field, GROUP_FIELD_SMALLER);
    ASSERT_EQ_INT(board.cursor, TEST_SECOND_SLOT);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_TRUE(slots_equal(&board, &before));
    ASSERT_TRUE(all_groups_empty(&board));
}

static void test_apply_board_input_move_up_restores_remembered_group_box(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    board.group_cursor = 9;
    board.group_field  = GROUP_FIELD_GREATER;
    press(&board, BOARD_INPUT_MOVE_UP);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    ASSERT_EQ_INT(board.group_cursor, 9);
    ASSERT_EQ_INT(board.group_field, GROUP_FIELD_GREATER);
}

static void
test_apply_board_input_move_down_from_last_group_row_returns_to_slot(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    press(&board, BOARD_INPUT_MOVE_UP);
    press_times(&board, BOARD_INPUT_MOVE_DOWN, TEST_GRID_LAST);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    ASSERT_EQ_INT(board.group_cursor, TEST_GRID_LAST * TETONOR_GRID_SIZE);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
    ASSERT_EQ_INT(board.cursor, TEST_SECOND_SLOT);
    ASSERT_EQ_INT(board.group_cursor, TEST_GRID_LAST * TETONOR_GRID_SIZE);
}

static void test_apply_board_input_move_down_from_group_keeps_the_field(void)
{
    for (int f = 0; f < TEST_FIELD_COUNT; f++)
    {
        NumberGeneration puzzle;
        Board board;
        start_default_board(&puzzle, &board);
        place_group_focus(&board, 5, fixture_fields[f]);
        press(&board, BOARD_INPUT_MOVE_DOWN);
        ASSERT_EQ_INT(board.group_cursor, 9);
        ASSERT_EQ_INT(board.group_field, fixture_fields[f]);
        press(&board, BOARD_INPUT_MOVE_DOWN);
        ASSERT_EQ_INT(board.group_cursor, 13);
        ASSERT_EQ_INT(board.group_field, fixture_fields[f]);
        ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    }
}

static void test_apply_board_input_move_up_from_group_keeps_the_field(void)
{
    for (int f = 0; f < TEST_FIELD_COUNT; f++)
    {
        NumberGeneration puzzle;
        Board board;
        start_default_board(&puzzle, &board);
        place_group_focus(&board, 10, fixture_fields[f]);
        press(&board, BOARD_INPUT_MOVE_UP);
        ASSERT_EQ_INT(board.group_cursor, 6);
        ASSERT_EQ_INT(board.group_field, fixture_fields[f]);
        press(&board, BOARD_INPUT_MOVE_UP);
        ASSERT_EQ_INT(board.group_cursor, 2);
        ASSERT_EQ_INT(board.group_field, fixture_fields[f]);
        ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    }
}

static void test_apply_board_input_move_up_on_first_group_row_stays(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 2, GROUP_FIELD_OPERATION);
    Board before = board;
    press(&board, BOARD_INPUT_MOVE_UP);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void
test_apply_board_input_move_down_on_last_group_row_leaves_groups(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, TEST_LAST_CELL, GROUP_FIELD_GREATER);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
    ASSERT_EQ_INT(board.cursor, TEST_FIRST_SLOT);
    ASSERT_EQ_INT(board.group_cursor, TEST_LAST_CELL);
    ASSERT_EQ_INT(board.group_field, GROUP_FIELD_GREATER);
}

static void test_apply_board_input_move_right_walks_the_boxes_of_a_row(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 0, GROUP_FIELD_SMALLER);
    for (int box = 0; box < TETONOR_GRID_SIZE * TEST_FIELD_COUNT; box++)
    {
        ASSERT_EQ_INT(board.group_cursor, box / TEST_FIELD_COUNT);
        ASSERT_EQ_INT(board.group_field,
                      fixture_fields[box % TEST_FIELD_COUNT]);
        press(&board, BOARD_INPUT_MOVE_RIGHT);
    }
    ASSERT_EQ_INT(board.group_cursor, TEST_GRID_LAST);
    ASSERT_EQ_INT(board.group_field, GROUP_FIELD_GREATER);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
}

static void test_apply_board_input_move_left_walks_the_boxes_of_a_row(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, TEST_GRID_LAST, GROUP_FIELD_GREATER);
    for (int box = TETONOR_GRID_SIZE * TEST_FIELD_COUNT - 1; box >= 0; box--)
    {
        ASSERT_EQ_INT(board.group_cursor, box / TEST_FIELD_COUNT);
        ASSERT_EQ_INT(board.group_field,
                      fixture_fields[box % TEST_FIELD_COUNT]);
        press(&board, BOARD_INPUT_MOVE_LEFT);
    }
    ASSERT_EQ_INT(board.group_cursor, 0);
    ASSERT_EQ_INT(board.group_field, GROUP_FIELD_SMALLER);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
}

static void
test_apply_board_input_move_right_from_greater_enters_next_group(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 1, GROUP_FIELD_GREATER);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    ASSERT_EQ_INT(board.group_cursor, 2);
    ASSERT_EQ_INT(board.group_field, GROUP_FIELD_SMALLER);
}

static void
test_apply_board_input_move_left_from_smaller_enters_previous_group(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 2, GROUP_FIELD_SMALLER);
    press(&board, BOARD_INPUT_MOVE_LEFT);
    ASSERT_EQ_INT(board.group_cursor, 1);
    ASSERT_EQ_INT(board.group_field, GROUP_FIELD_GREATER);
}

static void test_apply_board_input_move_right_at_row_end_does_not_wrap(void)
{
    for (int row = 0; row < TETONOR_GRID_SIZE; row++)
    {
        NumberGeneration puzzle;
        Board board;
        start_default_board(&puzzle, &board);
        int last_in_row = row * TETONOR_GRID_SIZE + TEST_GRID_LAST;
        place_group_focus(&board, last_in_row, GROUP_FIELD_GREATER);
        Board before = board;
        press(&board, BOARD_INPUT_MOVE_RIGHT);
        ASSERT_TRUE(boards_equal(&board, &before));
    }
}

static void test_apply_board_input_move_left_at_row_start_does_not_wrap(void)
{
    for (int row = 0; row < TETONOR_GRID_SIZE; row++)
    {
        NumberGeneration puzzle;
        Board board;
        start_default_board(&puzzle, &board);
        place_group_focus(&board, row * TETONOR_GRID_SIZE, GROUP_FIELD_SMALLER);
        Board before = board;
        press(&board, BOARD_INPUT_MOVE_LEFT);
        ASSERT_TRUE(boards_equal(&board, &before));
    }
}

static void test_apply_board_input_group_movement_keeps_every_value(void)
{
    NumberGeneration puzzle;
    Board start;
    start_correct_board(&puzzle, &start);
    ASSERT_EQ_INT(start.result, BOARD_RESULT_CORRECT);
    for (int sequence = 0; sequence < TEST_MOVE_SEQUENCE_COUNT; sequence++)
    {
        Board board = start;
        press_movement_sequence(&board, sequence);
        ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
        ASSERT_TRUE(slots_equal(&board, &start));
        ASSERT_TRUE(groups_equal(board.groups, start.groups));
    }
}

static void test_apply_board_input_move_up_from_submit_returns_to_slot(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    press(&board, BOARD_INPUT_MOVE_UP);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
    ASSERT_EQ_INT(board.cursor, TEST_SECOND_SLOT);
}

static void test_apply_board_input_move_left_from_submit_returns_to_slot(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    press(&board, BOARD_INPUT_MOVE_LEFT);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
    ASSERT_EQ_INT(board.cursor, TEST_SECOND_SLOT);
}

static void test_apply_board_input_move_down_on_submit_focuses_restart(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 6, GROUP_FIELD_OPERATION);
    board.cursor     = TEST_SECOND_SLOT;
    board.focus      = BOARD_FOCUS_SUBMIT;
    Board before     = board;
    BoardEvent event = press_for_event(&board, BOARD_INPUT_MOVE_DOWN);
    ASSERT_EQ_INT(event, BOARD_EVENT_NONE);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_RESTART);
    ASSERT_TRUE(equal_apart_from_focus(&board, &before));
}

static void test_apply_board_input_move_right_on_submit_is_noop(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    Board before = board;
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void test_apply_board_input_submit_up_keeps_remembered_group_box(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 7, GROUP_FIELD_OPERATION);
    board.focus = BOARD_FOCUS_SUBMIT;
    press(&board, BOARD_INPUT_MOVE_UP);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
    press(&board, BOARD_INPUT_MOVE_UP);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    ASSERT_EQ_INT(board.group_cursor, 7);
    ASSERT_EQ_INT(board.group_field, GROUP_FIELD_OPERATION);
}

static void test_apply_board_input_digit_on_empty_slot_sets_value(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 7);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 7);
}

static void test_apply_board_input_digit_zero_is_a_value(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 0);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 0);
}

static void test_apply_board_input_digit_only_changes_cursor_slot(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 7);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (board.slots[i].editable && i != TEST_FIRST_SLOT)
        {
            ASSERT_EQ_INT(board.slots[i].value, TETONOR_EMPTY_ENTRY);
        }
    }
}

static void test_apply_board_input_slot_digit_leaves_groups_empty(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    type_value(&board, 29);
    press(&board, BOARD_INPUT_DELETE);
    press_operation(&board, GROUP_OPERATION_ADD);
    ASSERT_TRUE(all_groups_empty(&board));
}

static void test_apply_board_input_two_digits_form_two_digit_number(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 1);
    press_digit(&board, 5);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 15);
}

static void test_apply_board_input_digits_reach_maximum_value(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 2);
    press_digit(&board, 9);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TETONOR_MAX_VALUE);
}

static void test_apply_board_input_digit_above_maximum_is_ignored(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 3);
    press_digit(&board, 5);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 3);
}

static void test_apply_board_input_digit_just_above_maximum_is_ignored(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 3);
    press_digit(&board, 0);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 3);
}

static void test_apply_board_input_digit_after_two_digits_is_ignored(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 2);
    press_digit(&board, 9);
    press_digit(&board, 1);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TETONOR_MAX_VALUE);
}

static void test_apply_board_input_digit_after_ten_is_ignored(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 1);
    press_digit(&board, 0);
    press_digit(&board, 0);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 10);
}

static void test_apply_board_input_leading_zero_is_replaced_by_next_digit(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 0);
    press_digit(&board, 7);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 7);
}

static void test_apply_board_input_repeated_zero_stays_zero(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 0);
    press_digit(&board, 0);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 0);
}

static void test_apply_board_input_digit_out_of_range_is_ignored(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, TEST_OUT_OF_RANGE_DIGIT);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TETONOR_EMPTY_ENTRY);
    press_digit(&board, -1);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TETONOR_EMPTY_ENTRY);
}

static void test_apply_board_input_digit_out_of_range_keeps_typed_value(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 5);
    press_digit(&board, TEST_OUT_OF_RANGE_DIGIT);
    press_digit(&board, -1);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 5);
}

static void test_apply_board_input_delete_removes_last_digit(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 2);
    press_digit(&board, 9);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 2);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TETONOR_EMPTY_ENTRY);
}

static void test_apply_board_input_delete_on_empty_slot_is_noop(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board before = board;
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void test_apply_board_input_delete_on_zero_empties_slot(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 0);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TETONOR_EMPTY_ENTRY);
}

static void test_apply_board_input_delete_on_ten_leaves_one(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 1);
    press_digit(&board, 0);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 1);
}

static void test_apply_board_input_typing_resumes_after_delete(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 2);
    press_digit(&board, 9);
    press(&board, BOARD_INPUT_DELETE);
    press_digit(&board, 4);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 24);
}

static void test_apply_board_input_digit_on_submit_focus_is_noop(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    /* 2 followed by any digit stays within the maximum, so a digit that leaked
     * through to the slot would change it. */
    press_digit(&board, 2);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    Board before = board;
    for (int digit = 0; digit < TETONOR_DIGIT_BASE; digit++)
    {
        press_digit(&board, digit);
        ASSERT_TRUE(boards_equal(&board, &before));
    }
}

static void test_apply_board_input_delete_on_submit_focus_is_noop(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 2);
    press_digit(&board, 9);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    Board before = board;
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_TRUE(boards_equal(&board, &before));
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TETONOR_MAX_VALUE);
}

static void test_apply_board_input_activate_on_slot_is_noop(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board before = board;
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_TRUE(boards_equal(&board, &before));
    press_digit(&board, 2);
    before = board;
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_TRUE(boards_equal(&board, &before));
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 2);
}

static void
test_apply_board_input_activate_on_slot_with_full_answer_is_noop(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_solution_groups(&board);
    fill_slots(&board, fixture_hidden_values);
    press(&board, BOARD_INPUT_MOVE_UP);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
    Board before = board;
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void test_apply_board_input_activate_on_group_is_noop(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_solution_groups(&board);
    fill_slots(&board, fixture_hidden_values);
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        for (int f = 0; f < TEST_FIELD_COUNT; f++)
        {
            place_group_focus(&board, cell, fixture_fields[f]);
            Board before = board;
            press(&board, BOARD_INPUT_ACTIVATE);
            ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
            ASSERT_TRUE(boards_equal(&board, &before));
        }
    }
}

static void test_apply_board_input_activate_on_group_keeps_result(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    place_group_focus(&board, 3, GROUP_FIELD_OPERATION);
    Board before = board;
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void test_apply_board_input_group_digit_sets_value(void)
{
    for (int f = 0; f < TEST_NUMBER_FIELD_COUNT; f++)
    {
        NumberGeneration puzzle;
        Board board;
        start_default_board(&puzzle, &board);
        place_group_focus(&board, 6, fixture_number_fields[f]);
        press_digit(&board, 7);
        ASSERT_TRUE(
            only_group_number_is_set(&board, 6, fixture_number_fields[f], 7));
    }
}

static void test_apply_board_input_group_digit_zero_is_a_value(void)
{
    for (int f = 0; f < TEST_NUMBER_FIELD_COUNT; f++)
    {
        NumberGeneration puzzle;
        Board board;
        start_default_board(&puzzle, &board);
        place_group_focus(&board, 0, fixture_number_fields[f]);
        press_digit(&board, 0);
        ASSERT_TRUE(
            only_group_number_is_set(&board, 0, fixture_number_fields[f], 0));
    }
}

static void test_apply_board_input_group_digit_does_not_touch_slots(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board before = board;
    place_group_focus(&board, 4, GROUP_FIELD_SMALLER);
    type_value(&board, 29);
    ASSERT_TRUE(slots_equal(&board, &before));
    ASSERT_EQ_INT(board.cursor, TEST_FIRST_SLOT);
}

static void test_apply_board_input_group_two_digits_form_number(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 15, GROUP_FIELD_GREATER);
    press_digit(&board, 1);
    press_digit(&board, 5);
    ASSERT_EQ_INT(board.groups[15].greater, 15);
}

static void test_apply_board_input_group_digits_reach_maximum_value(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 15, GROUP_FIELD_GREATER);
    type_value(&board, TETONOR_MAX_VALUE);
    ASSERT_EQ_INT(board.groups[15].greater, TETONOR_MAX_VALUE);
}

static void test_apply_board_input_group_digit_above_maximum_is_ignored(void)
{
    for (int f = 0; f < TEST_NUMBER_FIELD_COUNT; f++)
    {
        NumberGeneration puzzle;
        Board board;
        start_default_board(&puzzle, &board);
        place_group_focus(&board, 8, fixture_number_fields[f]);
        press_digit(&board, 3);
        press_digit(&board, 5);
        ASSERT_EQ_INT(group_number(&board, 8, fixture_number_fields[f]), 3);
        press_digit(&board, 0);
        ASSERT_EQ_INT(group_number(&board, 8, fixture_number_fields[f]), 3);
    }
}

static void test_apply_board_input_group_digit_after_two_digits_is_ignored(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 1, GROUP_FIELD_SMALLER);
    press_digit(&board, 1);
    press_digit(&board, 0);
    press_digit(&board, 0);
    ASSERT_EQ_INT(board.groups[1].smaller, 10);
    press_digit(&board, 5);
    ASSERT_EQ_INT(board.groups[1].smaller, 10);
}

static void test_apply_board_input_group_leading_zero_is_replaced(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 2, GROUP_FIELD_GREATER);
    press_digit(&board, 0);
    press_digit(&board, 7);
    ASSERT_EQ_INT(board.groups[2].greater, 7);
}

static void test_apply_board_input_group_repeated_zero_stays_zero(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 2, GROUP_FIELD_GREATER);
    press_digit(&board, 0);
    press_digit(&board, 0);
    ASSERT_EQ_INT(board.groups[2].greater, 0);
}

static void test_apply_board_input_group_digit_out_of_range_is_ignored(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 2, GROUP_FIELD_SMALLER);
    press_digit(&board, TEST_OUT_OF_RANGE_DIGIT);
    press_digit(&board, -1);
    ASSERT_TRUE(all_groups_empty(&board));
    press_digit(&board, 5);
    press_digit(&board, TEST_OUT_OF_RANGE_DIGIT);
    press_digit(&board, -1);
    ASSERT_EQ_INT(board.groups[2].smaller, 5);
}

static void test_apply_board_input_group_digit_after_result_clears_result(void)
{
    NumberGeneration puzzle;
    Board board;
    start_wrong_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    place_group_focus(&board, 3, GROUP_FIELD_GREATER);
    press_digit(&board, 7);
    ASSERT_EQ_INT(board.groups[3].greater, 7);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void test_apply_board_input_group_zero_after_result_replaces_value(void)
{
    NumberGeneration puzzle;
    Board board;
    start_wrong_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    place_group_focus(&board, 3, GROUP_FIELD_SMALLER);
    press_digit(&board, 0);
    ASSERT_EQ_INT(board.groups[3].smaller, 0);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void test_apply_board_input_group_digit_out_of_range_keeps_result(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    place_group_focus(&board, 3, GROUP_FIELD_GREATER);
    Board before = board;
    press_digit(&board, TEST_OUT_OF_RANGE_DIGIT);
    press_digit(&board, -1);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void test_apply_board_input_group_delete_removes_last_digit(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 5, GROUP_FIELD_GREATER);
    type_value(&board, 29);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.groups[5].greater, 2);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.groups[5].greater, TETONOR_EMPTY_ENTRY);
}

static void test_apply_board_input_group_delete_on_empty_number_is_noop(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 5, GROUP_FIELD_SMALLER);
    Board before = board;
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void test_apply_board_input_group_delete_on_zero_empties_box(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 5, GROUP_FIELD_SMALLER);
    press_digit(&board, 0);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.groups[5].smaller, TETONOR_EMPTY_ENTRY);
}

static void test_apply_board_input_group_delete_on_ten_leaves_one(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 5, GROUP_FIELD_SMALLER);
    type_value(&board, 10);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.groups[5].smaller, 1);
}

static void test_apply_board_input_group_typing_resumes_after_delete(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 5, GROUP_FIELD_GREATER);
    type_value(&board, 29);
    press(&board, BOARD_INPUT_DELETE);
    press_digit(&board, 4);
    ASSERT_EQ_INT(board.groups[5].greater, 24);
}

static void test_apply_board_input_group_delete_after_result_clears_result(void)
{
    NumberGeneration puzzle;
    Board board;
    start_wrong_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    place_group_focus(&board, 13, GROUP_FIELD_SMALLER);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.groups[13].smaller, 1);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void
test_apply_board_input_group_delete_after_result_empties_single_digit(void)
{
    NumberGeneration puzzle;
    Board board;
    start_wrong_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    place_group_focus(&board, 0, GROUP_FIELD_GREATER);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.groups[0].greater, TETONOR_EMPTY_ENTRY);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void test_apply_board_input_operation_sets_the_operation_box(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 9, GROUP_FIELD_OPERATION);
    press_operation(&board, GROUP_OPERATION_ADD);
    ASSERT_EQ_INT(board.groups[9].operation, GROUP_OPERATION_ADD);
    press_operation(&board, GROUP_OPERATION_MULTIPLY);
    ASSERT_EQ_INT(board.groups[9].operation, GROUP_OPERATION_MULTIPLY);
    press_operation(&board, GROUP_OPERATION_MULTIPLY);
    ASSERT_EQ_INT(board.groups[9].operation, GROUP_OPERATION_MULTIPLY);
    press_operation(&board, GROUP_OPERATION_ADD);
    ASSERT_EQ_INT(board.groups[9].operation, GROUP_OPERATION_ADD);
}

static void
test_apply_board_input_operation_only_changes_the_focused_group(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 9, GROUP_FIELD_OPERATION);
    press_operation(&board, GROUP_OPERATION_MULTIPLY);
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        GroupOperation expected =
            cell == 9 ? GROUP_OPERATION_MULTIPLY : GROUP_OPERATION_NONE;
        ASSERT_EQ_INT(board.groups[cell].operation, expected);
        ASSERT_EQ_INT(board.groups[cell].smaller, TETONOR_EMPTY_ENTRY);
        ASSERT_EQ_INT(board.groups[cell].greater, TETONOR_EMPTY_ENTRY);
    }
}

static void test_apply_board_input_operation_none_is_ignored(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 9, GROUP_FIELD_OPERATION);
    Board before = board;
    press_operation(&board, GROUP_OPERATION_NONE);
    ASSERT_TRUE(boards_equal(&board, &before));
    press_operation(&board, GROUP_OPERATION_ADD);
    before = board;
    press_operation(&board, GROUP_OPERATION_NONE);
    ASSERT_TRUE(boards_equal(&board, &before));
    ASSERT_EQ_INT(board.groups[9].operation, GROUP_OPERATION_ADD);
}

static void
test_apply_board_input_operation_none_after_result_keeps_result(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    place_group_focus(&board, 0, GROUP_FIELD_OPERATION);
    Board before = board;
    press_operation(&board, GROUP_OPERATION_NONE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void test_apply_board_input_operation_after_result_clears_result(void)
{
    NumberGeneration puzzle;
    Board board;
    start_wrong_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    place_group_focus(&board, 0, GROUP_FIELD_OPERATION);
    press_operation(&board, GROUP_OPERATION_MULTIPLY);
    ASSERT_EQ_INT(board.groups[0].operation, GROUP_OPERATION_MULTIPLY);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void
test_apply_board_input_same_operation_after_result_clears_result(void)
{
    NumberGeneration puzzle;
    Board board;
    start_wrong_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    place_group_focus(&board, 0, GROUP_FIELD_OPERATION);
    press_operation(&board, GROUP_OPERATION_ADD);
    ASSERT_EQ_INT(board.groups[0].operation, GROUP_OPERATION_ADD);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void test_apply_board_input_delete_clears_the_operation_box(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 9, GROUP_FIELD_OPERATION);
    press_operation(&board, GROUP_OPERATION_ADD);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.groups[9].operation, GROUP_OPERATION_NONE);
    ASSERT_TRUE(all_groups_empty(&board));
}

static void test_apply_board_input_delete_on_empty_operation_is_noop(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 9, GROUP_FIELD_OPERATION);
    Board before = board;
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void
test_apply_board_input_delete_on_operation_after_result_clears_it(void)
{
    NumberGeneration puzzle;
    Board board;
    start_wrong_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    place_group_focus(&board, 4, GROUP_FIELD_OPERATION);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.groups[4].operation, GROUP_OPERATION_NONE);
    ASSERT_EQ_INT(board.groups[4].smaller, fixture_groups[4].smaller);
    ASSERT_EQ_INT(board.groups[4].greater, fixture_groups[4].greater);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void test_apply_board_input_operation_on_group_number_is_ignored(void)
{
    NumberGeneration puzzle;
    Board start;
    start_correct_board(&puzzle, &start);
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        for (int f = 0; f < TEST_NUMBER_FIELD_COUNT; f++)
        {
            Board board = start;
            place_group_focus(&board, cell, fixture_number_fields[f]);
            Board before = board;
            press_operation(&board, GROUP_OPERATION_ADD);
            press_operation(&board, GROUP_OPERATION_MULTIPLY);
            press_operation(&board, GROUP_OPERATION_NONE);
            ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
            ASSERT_TRUE(boards_equal(&board, &before));
        }
    }
}

static void
test_apply_board_input_operation_on_empty_group_number_is_ignored(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 0, GROUP_FIELD_SMALLER);
    press_operation(&board, GROUP_OPERATION_ADD);
    place_group_focus(&board, 0, GROUP_FIELD_GREATER);
    press_operation(&board, GROUP_OPERATION_MULTIPLY);
    ASSERT_TRUE(all_groups_empty(&board));
}

static void test_apply_board_input_operation_on_slot_is_ignored(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 2);
    Board before = board;
    press_operation(&board, GROUP_OPERATION_ADD);
    press_operation(&board, GROUP_OPERATION_MULTIPLY);
    ASSERT_TRUE(boards_equal(&board, &before));
    ASSERT_TRUE(all_groups_empty(&board));
}

static void
test_apply_board_input_operation_on_slot_after_result_keeps_result(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
    Board before = board;
    press_operation(&board, GROUP_OPERATION_ADD);
    press_operation(&board, GROUP_OPERATION_MULTIPLY);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void test_apply_board_input_operation_on_submit_is_ignored(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
    Board before = board;
    press_operation(&board, GROUP_OPERATION_ADD);
    press_operation(&board, GROUP_OPERATION_MULTIPLY);
    press_operation(&board, GROUP_OPERATION_NONE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void test_apply_board_input_digit_on_operation_box_is_ignored(void)
{
    NumberGeneration puzzle;
    Board start;
    start_correct_board(&puzzle, &start);
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        Board board = start;
        place_group_focus(&board, cell, GROUP_FIELD_OPERATION);
        Board before = board;
        for (int digit = 0; digit < TETONOR_DIGIT_BASE; digit++)
        {
            press_digit(&board, digit);
            ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
            ASSERT_TRUE(boards_equal(&board, &before));
        }
        press_digit(&board, TEST_OUT_OF_RANGE_DIGIT);
        press_digit(&board, -1);
        ASSERT_TRUE(boards_equal(&board, &before));
    }
}

static void test_apply_board_input_digit_on_empty_operation_box_is_ignored(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 6, GROUP_FIELD_OPERATION);
    Board before = board;
    press_digit(&board, 5);
    ASSERT_TRUE(boards_equal(&board, &before));
    ASSERT_TRUE(all_groups_empty(&board));
}

static void test_apply_board_input_group_by_inputs_enters_a_full_group(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    type_value(&board, 11);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    press_operation(&board, GROUP_OPERATION_MULTIPLY);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    type_value(&board, 12);
    ASSERT_EQ_INT(board.groups[0].smaller, 11);
    ASSERT_EQ_INT(board.groups[0].operation, GROUP_OPERATION_MULTIPLY);
    ASSERT_EQ_INT(board.groups[0].greater, 12);
    ASSERT_EQ_INT(board.group_cursor, 0);
    ASSERT_EQ_INT(board.group_field, GROUP_FIELD_GREATER);
}

static void enter_group_by_inputs(Board* board, const OperandGroup* group)
{
    type_value(board, group->smaller);
    press(board, BOARD_INPUT_MOVE_RIGHT);
    press_operation(board, group->operation);
    press(board, BOARD_INPUT_MOVE_RIGHT);
    type_value(board, group->greater);
}

/* Starts on the first box of the first group and leaves the focus on the
 * greater box of the last group. A row is left by DOWN and walked back by
 * LEFT because RIGHT never wraps. */
static void enter_all_groups_by_inputs(Board* board)
{
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        enter_group_by_inputs(board, &fixture_groups[cell]);
        if (cell == TEST_LAST_CELL)
        {
            continue;
        }
        if (cell % TETONOR_GRID_SIZE != TEST_GRID_LAST)
        {
            press(board, BOARD_INPUT_MOVE_RIGHT);
            continue;
        }
        press(board, BOARD_INPUT_MOVE_DOWN);
        press_times(board, BOARD_INPUT_MOVE_LEFT,
                    TETONOR_GRID_SIZE * TEST_FIELD_COUNT - 1);
    }
}

static void test_apply_board_input_whole_board_by_inputs_is_correct(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_all_groups_by_inputs(&board);
    ASSERT_TRUE(groups_equal(board.groups, fixture_groups));
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    ASSERT_EQ_INT(board.group_cursor, TEST_LAST_CELL);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
    ASSERT_EQ_INT(board.cursor, TEST_FIRST_SLOT);
    fill_slots(&board, fixture_hidden_values);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void test_apply_board_input_submit_accepts_zero_entry(void)
{
    NumberGeneration puzzle;
    Board board;
    const int hidden[] = {0};
    build_fixture_puzzle(&puzzle, hidden, 1);
    init_board(&board, &puzzle);
    fill_solution_groups(&board);
    press_digit(&board, 0);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.slots[0].value, 0);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void test_apply_board_input_submit_judges_single_editable_slot(void)
{
    NumberGeneration puzzle;
    Board board;
    const int hidden[] = {TEST_THIRD_SLOT};
    build_fixture_puzzle(&puzzle, hidden, 1);
    init_board(&board, &puzzle);
    fill_solution_groups(&board);
    press_digit(&board, 5);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    press(&board, BOARD_INPUT_MOVE_UP);
    press_digit(&board, 6);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void test_apply_board_input_submit_twice_keeps_result(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    Board before = board;
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_TRUE(boards_equal(&board, &before));
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void test_apply_board_input_submit_twice_keeps_wrong_result(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_and_submit(&board, fixture_wrong_last_values);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    Board before = board;
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_TRUE(boards_equal(&board, &before));
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
}

static void test_apply_board_input_submit_with_empty_slot_stays_pending(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_solution_groups(&board);
    for (int i = 0; i < TEST_EDITABLE_COUNT - 1; i++)
    {
        type_value(&board, fixture_hidden_values[i]);
        press(&board, BOARD_INPUT_MOVE_RIGHT);
    }
    press(&board, BOARD_INPUT_MOVE_DOWN);
    Board before = board;
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void
test_apply_board_input_submit_with_empty_first_slot_stays_pending(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_solution_groups(&board);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    for (int i = 1; i < TEST_EDITABLE_COUNT; i++)
    {
        type_value(&board, fixture_hidden_values[i]);
        press(&board, BOARD_INPUT_MOVE_RIGHT);
    }
    Board before = board;
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_TRUE(boards_equal(&board, &before));
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TETONOR_EMPTY_ENTRY);
}

static void test_apply_board_input_submit_with_empty_groups_stays_pending(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_slots(&board, fixture_hidden_values);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
    Board before = board;
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void
test_apply_board_input_submit_with_any_empty_group_box_stays_pending(void)
{
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        for (int f = 0; f < TEST_FIELD_COUNT; f++)
        {
            NumberGeneration puzzle;
            Board board;
            start_default_board(&puzzle, &board);
            fill_solution_groups(&board);
            empty_group_field(&board, cell, fixture_fields[f]);
            fill_slots(&board, fixture_hidden_values);
            Board before = board;
            press(&board, BOARD_INPUT_ACTIVATE);
            ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
            ASSERT_TRUE(boards_equal(&board, &before));
        }
    }
}

static void test_apply_board_input_typing_never_judges(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_solution_groups(&board);
    type_value(&board, TEST_WRONG_VALUE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    for (int i = 1; i < TEST_EDITABLE_COUNT; i++)
    {
        press(&board, BOARD_INPUT_MOVE_RIGHT);
        type_value(&board, TEST_WRONG_VALUE);
        ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    }
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void test_apply_board_input_all_filled_without_submit_stays_pending(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_solution_groups(&board);
    fill_slots(&board, fixture_hidden_values);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
}

static void test_apply_board_input_all_correct_is_correct(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
}

static void test_apply_board_input_one_wrong_entry_is_wrong(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_and_submit(&board, fixture_wrong_last_values);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
}

static void test_apply_board_input_swapped_entries_are_wrong(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    const int values[TEST_EDITABLE_COUNT] = {5, 2, 6, 29};
    fill_and_submit(&board, values);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
}

static void test_apply_board_input_filling_in_reverse_order_is_correct(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_solution_groups(&board);
    for (int i = 0; i < TEST_EDITABLE_COUNT - 1; i++)
    {
        press(&board, BOARD_INPUT_MOVE_RIGHT);
    }
    for (int i = TEST_EDITABLE_COUNT - 1; i >= 0; i--)
    {
        type_value(&board, fixture_hidden_values[i]);
        press(&board, BOARD_INPUT_MOVE_LEFT);
    }
    press(&board, BOARD_INPUT_MOVE_DOWN);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void test_apply_board_input_duplicate_of_given_value_is_accepted(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    const int values[TEST_EDITABLE_COUNT] = {1, 5, 6, 29};
    fill_and_submit(&board, values);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 1);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
}

static void test_apply_board_input_duplicate_entries_are_accepted(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    const int values[TEST_EDITABLE_COUNT] = {5, 5, 5, 5};
    fill_and_submit(&board, values);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 5);
    ASSERT_EQ_INT(board.slots[TEST_SECOND_SLOT].value, 5);
    ASSERT_EQ_INT(board.slots[TEST_THIRD_SLOT].value, 5);
    ASSERT_EQ_INT(board.slots[TEST_LAST_SLOT].value, 5);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
}

static void test_apply_board_input_wrong_operation_is_wrong(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_solution_groups(&board);
    board.groups[0].operation = GROUP_OPERATION_MULTIPLY;
    fill_slots(&board, fixture_hidden_values);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
}

static void test_apply_board_input_smaller_above_greater_is_wrong(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_solution_groups(&board);
    board.groups[0] = (OperandGroup){2, GROUP_OPERATION_ADD, 1};
    board.groups[1] = (OperandGroup){2, GROUP_OPERATION_MULTIPLY, 1};
    fill_slots(&board, fixture_hidden_values);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
}

static void test_apply_board_input_wrong_group_number_is_wrong(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_solution_groups(&board);
    board.groups[TEST_LAST_CELL].greater = TETONOR_MAX_VALUE - 1;
    fill_slots(&board, fixture_hidden_values);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
}

static void
test_apply_board_input_accepts_a_solution_other_than_the_generators(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    /* Fixture F-B: the first two pairs are (1,4) and (2,3), assigned to cells 0
     * and 2 the other way round from puzzle.solution_groups. */
    const int alternative_sums[] = {5, 4, 5, 6};
    for (int cell = 0; cell < 4; cell++)
    {
        puzzle.sum_matrix[0][cell] = alternative_sums[cell];
    }
    fill_solution_groups(&board);
    board.groups[0] = (OperandGroup){2, GROUP_OPERATION_ADD, 3};
    board.groups[1] = (OperandGroup){1, GROUP_OPERATION_MULTIPLY, 4};
    board.groups[2] = (OperandGroup){1, GROUP_OPERATION_ADD, 4};
    board.groups[3] = (OperandGroup){2, GROUP_OPERATION_MULTIPLY, 3};
    fill_slots(&board, fixture_hidden_values);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void
test_apply_board_input_digit_after_result_replaces_value_and_clears_result(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_and_submit(&board, fixture_wrong_last_values);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    press(&board, BOARD_INPUT_MOVE_UP);
    press_digit(&board, 7);
    ASSERT_EQ_INT(board.slots[TEST_LAST_SLOT].value, 7);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void test_apply_board_input_zero_after_result_replaces_value(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_and_submit(&board, fixture_wrong_last_values);
    press(&board, BOARD_INPUT_MOVE_UP);
    press_digit(&board, 0);
    ASSERT_EQ_INT(board.slots[TEST_LAST_SLOT].value, 0);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void test_apply_board_input_edit_after_result_keeps_other_values(void)
{
    NumberGeneration puzzle;
    Board board;
    start_wrong_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    press(&board, BOARD_INPUT_MOVE_UP);
    press_digit(&board, 7);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 2);
    ASSERT_EQ_INT(board.slots[TEST_SECOND_SLOT].value, 5);
    ASSERT_EQ_INT(board.slots[TEST_THIRD_SLOT].value, 6);
    ASSERT_EQ_INT(board.slots[TEST_LAST_SLOT].value, 7);
    ASSERT_TRUE(groups_equal(board.groups, fixture_groups));
}

static void
test_apply_board_input_digit_out_of_range_after_result_keeps_result(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_and_submit(&board, fixture_wrong_last_values);
    press(&board, BOARD_INPUT_MOVE_UP);
    Board before = board;
    press_digit(&board, TEST_OUT_OF_RANGE_DIGIT);
    press_digit(&board, -1);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void
test_apply_board_input_delete_after_result_removes_digit_and_clears_result(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_and_submit(&board, fixture_wrong_last_values);
    press(&board, BOARD_INPUT_MOVE_UP);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.slots[TEST_LAST_SLOT].value, 2);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void
test_apply_board_input_delete_after_result_on_single_digit_empties(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    const int values[TEST_EDITABLE_COUNT] = {2, 5, 6, 4};
    fill_and_submit(&board, values);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    press(&board, BOARD_INPUT_MOVE_UP);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.slots[TEST_LAST_SLOT].value, TETONOR_EMPTY_ENTRY);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void test_apply_board_input_resubmitting_fixed_entry_is_correct(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_and_submit(&board, fixture_wrong_last_values);
    press(&board, BOARD_INPUT_MOVE_UP);
    press(&board, BOARD_INPUT_DELETE);
    press(&board, BOARD_INPUT_DELETE);
    type_value(&board, TETONOR_MAX_VALUE);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void test_apply_board_input_wrong_then_digit_fix_is_correct(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_and_submit(&board, fixture_wrong_last_values);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    press(&board, BOARD_INPUT_MOVE_UP);
    press_digit(&board, 2);
    press_digit(&board, 9);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void test_apply_board_input_wrong_then_group_fix_is_correct(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_solution_groups(&board);
    board.groups[0].operation = GROUP_OPERATION_MULTIPLY;
    fill_slots(&board, fixture_hidden_values);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    press(&board, BOARD_INPUT_MOVE_UP);
    press(&board, BOARD_INPUT_MOVE_UP);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    ASSERT_EQ_INT(board.group_field, GROUP_FIELD_OPERATION);
    press_operation(&board, GROUP_OPERATION_ADD);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    press_times(&board, BOARD_INPUT_MOVE_DOWN, TETONOR_GRID_SIZE + 1);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void test_apply_board_input_editing_correct_result_is_locked(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    press(&board, BOARD_INPUT_MOVE_UP);
    Board before = board;
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(boards_equal(&board, &before));
    press(&board, BOARD_INPUT_DELETE);
    type_value(&board, TEST_WRONG_VALUE);
    ASSERT_EQ_INT(board.slots[TEST_LAST_SLOT].value, 29);
    ASSERT_TRUE(boards_equal(&board, &before));
    press(&board, BOARD_INPUT_MOVE_DOWN);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    ASSERT_EQ_INT(board.slots[TEST_LAST_SLOT].value, 29);
}

static void test_apply_board_input_editing_a_group_clears_a_wrong_result(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_and_submit(&board, fixture_wrong_last_values);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    place_group_focus(&board, 0, GROUP_FIELD_OPERATION);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void assert_movement_keeps_result(const int values[TEST_EDITABLE_COUNT],
                                         BoardResult expected)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_and_submit(&board, values);
    ASSERT_EQ_INT(board.result, expected);
    Board before                     = board;
    const BoardInputKind movements[] = {
        BOARD_INPUT_MOVE_UP,    BOARD_INPUT_MOVE_LEFT, BOARD_INPUT_MOVE_RIGHT,
        BOARD_INPUT_MOVE_RIGHT, BOARD_INPUT_MOVE_DOWN, BOARD_INPUT_MOVE_UP,
        BOARD_INPUT_MOVE_UP,    BOARD_INPUT_MOVE_DOWN, BOARD_INPUT_MOVE_LEFT};
    for (size_t i = 0; i < sizeof movements / sizeof movements[0]; i++)
    {
        press(&board, movements[i]);
        ASSERT_EQ_INT(board.result, expected);
        ASSERT_TRUE(slots_equal(&board, &before));
        ASSERT_TRUE(groups_equal(board.groups, before.groups));
    }
}

static void test_apply_board_input_moving_cursor_keeps_result(void)
{
    assert_movement_keeps_result(fixture_hidden_values, BOARD_RESULT_CORRECT);
    assert_movement_keeps_result(fixture_wrong_last_values, BOARD_RESULT_WRONG);
}

static void test_apply_board_input_never_changes_given_numbers(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    int given_before[TETONOR_CELL_COUNT];
    memcpy(given_before, puzzle.available_array, sizeof given_before);
    const int wrong_values[TEST_EDITABLE_COUNT] = {1, 0, 14, 7};
    fill_and_submit(&board, wrong_values);
    press(&board, BOARD_INPUT_MOVE_UP);
    press_digit(&board, 9);
    press(&board, BOARD_INPUT_DELETE);
    for (int i = TEST_EDITABLE_COUNT - 1; i >= 0; i--)
    {
        retype_slot(&board, fixture_hidden_values[i]);
        press(&board, BOARD_INPUT_MOVE_LEFT);
    }
    press(&board, BOARD_INPUT_MOVE_DOWN);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(
        memcmp(given_before, puzzle.available_array, sizeof given_before) == 0);
    ASSERT_TRUE(memcmp(fixture_strip, puzzle.solution_array,
                       sizeof fixture_strip) == 0);
    ASSERT_TRUE(memcmp(fixture_groups, puzzle.solution_groups,
                       sizeof fixture_groups) == 0);
}

#define TEST_GIVEN_ONE_SLOT 1
#define TEST_GIVEN_ONE_VALUE 1
#define TEST_GIVEN_THREE_SLOT 3
#define TEST_UNUSED_VALUE 6
#define TEST_TYPED_TWO_DIGIT_VALUE 29
#define TEST_THIRD_GROUP_CELL 2
#define TEST_FOURTH_GROUP_CELL 3
#define TEST_GARBAGE_BYTE 0xA5
#define TEST_INPUT_KIND_COUNT 8
#define TEST_FOCUS_COUNT 4

static const OperandGroup usage_add_group        = {1, GROUP_OPERATION_ADD, 2};
static const OperandGroup usage_multiply_group   = {1, GROUP_OPERATION_MULTIPLY,
                                                    2};
static const OperandGroup usage_second_add_group = {1, GROUP_OPERATION_ADD, 3};
static const OperandGroup usage_third_add_group  = {1, GROUP_OPERATION_ADD, 4};
static const BoardInputKind fixture_input_kinds[TEST_INPUT_KIND_COUNT] = {
    BOARD_INPUT_MOVE_LEFT, BOARD_INPUT_MOVE_RIGHT, BOARD_INPUT_MOVE_UP,
    BOARD_INPUT_MOVE_DOWN, BOARD_INPUT_DIGIT,      BOARD_INPUT_DELETE,
    BOARD_INPUT_ACTIVATE,  BOARD_INPUT_OPERATION};
static const BoardFocus fixture_focuses[TEST_FOCUS_COUNT] = {
    BOARD_FOCUS_SLOT, BOARD_FOCUS_SUBMIT, BOARD_FOCUS_GROUP,
    BOARD_FOCUS_RESTART};

/* Leaves the focus on the greater box of the group. */
static void enter_group_in_cell(Board* board, int cell,
                                const OperandGroup* group)
{
    place_group_focus(board, cell, GROUP_FIELD_SMALLER);
    enter_group_by_inputs(board, group);
}

static void focus_slot(Board* board, int slot)
{
    board->focus  = BOARD_FOCUS_SLOT;
    board->cursor = slot;
}

static void set_all_usages(Board* board, SlotUsage usage)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        board->slot_usages[i] = usage;
    }
}

static bool all_usages_are(const Board* board, SlotUsage usage)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (board->slot_usages[i] != usage)
        {
            return false;
        }
    }
    return true;
}

static int marked_usage_count(const Board* board)
{
    int count = 0;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (board->slot_usages[i] != SLOT_USAGE_NONE)
        {
            count++;
        }
    }
    return count;
}

static bool palette_entries_equal(const PaletteEntry* first,
                                  const PaletteEntry* second)
{
    return first->color == second->color && first->fallback == second->fallback;
}

static bool palette_is_default(const Palette* palette)
{
    Palette expected;
    init_palette(&expected);
    for (int role = 0; role < PALETTE_ROLE_COUNT; role++)
    {
        if (!palette_entries_equal(&palette->foregrounds[role],
                                   &expected.foregrounds[role]))
        {
            return false;
        }
    }
    return palette_entries_equal(&palette->background, &expected.background);
}

static void test_init_board_slot_usages_start_unmarked(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    ASSERT_TRUE(all_usages_are(&board, SLOT_USAGE_NONE));
}

static void test_init_board_without_editable_slots_has_unmarked_usages(void)
{
    NumberGeneration puzzle;
    Board board;
    start_board_without_slots(&puzzle, &board);
    ASSERT_TRUE(all_usages_are(&board, SLOT_USAGE_NONE));
}

static void test_init_board_recomputes_usages_of_a_previous_session(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_group_by_inputs(&board, &usage_add_group);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_ONCE);
    set_all_usages(&board, SLOT_USAGE_TWICE);
    init_board(&board, &puzzle);
    ASSERT_TRUE(all_usages_are(&board, SLOT_USAGE_NONE));
}

static void test_init_board_fills_the_default_palette(void)
{
    NumberGeneration puzzle;
    Board board;
    build_fixture_puzzle(&puzzle, fixture_hidden, TEST_EDITABLE_COUNT);
    memset(&board.palette, TEST_GARBAGE_BYTE, sizeof board.palette);
    init_board(&board, &puzzle);
    ASSERT_TRUE(palette_is_default(&board.palette));
}

static void test_init_board_restores_the_default_palette_of_a_used_board(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    board.palette.foregrounds[PALETTE_ROLE_USED_ONCE].color =
        PALETTE_COLOR_MAGENTA;
    ASSERT_FALSE(palette_is_default(&board.palette));
    init_board(&board, &puzzle);
    ASSERT_TRUE(palette_is_default(&board.palette));
}

static void test_apply_board_input_group_entry_marks_the_strip_live(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    type_value(&board, usage_add_group.smaller);
    ASSERT_TRUE(all_usages_are(&board, SLOT_USAGE_NONE));
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    press_operation(&board, usage_add_group.operation);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(marked_usage_count(&board), 1);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    type_value(&board, usage_add_group.greater);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_ONCE);
}

static void test_apply_board_input_empty_hidden_slot_is_never_marked(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_group_by_inputs(&board, &usage_add_group);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TETONOR_EMPTY_ENTRY);
    ASSERT_EQ_INT(board.slot_usages[TEST_FIRST_SLOT], SLOT_USAGE_NONE);
    ASSERT_EQ_INT(marked_usage_count(&board), 1);
}

static void test_apply_board_input_typing_a_used_value_marks_it_at_once(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_group_by_inputs(&board, &usage_add_group);
    focus_slot(&board, TEST_FIRST_SLOT);
    press_digit(&board, 2);
    ASSERT_EQ_INT(board.slot_usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_ONCE);
}

static void test_apply_board_input_typing_a_twice_used_value_marks_twice(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_group_by_inputs(&board, &usage_add_group);
    enter_group_in_cell(&board, 1, &usage_multiply_group);
    ASSERT_EQ_INT(board.slot_usages[TEST_FIRST_SLOT], SLOT_USAGE_NONE);
    focus_slot(&board, TEST_FIRST_SLOT);
    press_digit(&board, 2);
    ASSERT_EQ_INT(board.slot_usages[TEST_FIRST_SLOT], SLOT_USAGE_TWICE);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_TWICE);
    ASSERT_EQ_INT(marked_usage_count(&board), 2);
}

static void test_apply_board_input_deleting_a_typed_value_unmarks_it(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_group_by_inputs(&board, &usage_add_group);
    focus_slot(&board, TEST_FIRST_SLOT);
    press_digit(&board, 2);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.slot_usages[TEST_FIRST_SLOT], SLOT_USAGE_NONE);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_ONCE);
}

static void
test_apply_board_input_extending_a_typed_value_updates_the_mark(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_group_by_inputs(&board, &usage_add_group);
    focus_slot(&board, TEST_FIRST_SLOT);
    press_digit(&board, 2);
    ASSERT_EQ_INT(board.slot_usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
    press_digit(&board, 9);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value,
                  TEST_TYPED_TWO_DIGIT_VALUE);
    ASSERT_EQ_INT(board.slot_usages[TEST_FIRST_SLOT], SLOT_USAGE_NONE);
}

static void test_apply_board_input_typed_unused_value_stays_unmarked(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_group_by_inputs(&board, &usage_add_group);
    focus_slot(&board, TEST_FIRST_SLOT);
    press_digit(&board, TEST_UNUSED_VALUE);
    ASSERT_EQ_INT(board.slot_usages[TEST_FIRST_SLOT], SLOT_USAGE_NONE);
}

static void test_apply_board_input_add_and_multiply_group_mark_twice(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_group_by_inputs(&board, &usage_add_group);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_ONCE);
    enter_group_in_cell(&board, 1, &usage_multiply_group);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_TWICE);
}

static void test_apply_board_input_second_use_of_one_operation_stays_once(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_group_by_inputs(&board, &usage_add_group);
    enter_group_in_cell(&board, 1, &usage_second_add_group);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_THREE_SLOT], SLOT_USAGE_ONCE);
}

static void test_apply_board_input_third_use_keeps_the_box_twice(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_group_by_inputs(&board, &usage_add_group);
    enter_group_in_cell(&board, 1, &usage_multiply_group);
    enter_group_in_cell(&board, TEST_THIRD_GROUP_CELL, &usage_second_add_group);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_TWICE);
    enter_group_in_cell(&board, TEST_FOURTH_GROUP_CELL, &usage_third_add_group);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_TWICE);
}

static void test_apply_board_input_two_adds_mark_equal_boxes_both_once(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_group_by_inputs(&board, &usage_second_add_group);
    enter_group_in_cell(&board, 1, &usage_third_add_group);
    focus_slot(&board, TEST_FIRST_SLOT);
    press_digit(&board, TEST_GIVEN_ONE_VALUE);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(board.slot_usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
}

static void test_apply_board_input_equal_boxes_split_add_and_multiply(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_group_by_inputs(&board, &usage_add_group);
    enter_group_in_cell(&board, 1, &usage_multiply_group);
    focus_slot(&board, TEST_FIRST_SLOT);
    press_digit(&board, TEST_GIVEN_ONE_VALUE);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_TWICE);
    ASSERT_EQ_INT(board.slot_usages[TEST_FIRST_SLOT], SLOT_USAGE_NONE);
}

static void test_apply_board_input_equal_boxes_two_adds_and_a_multiply(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_group_by_inputs(&board, &usage_add_group);
    enter_group_in_cell(&board, 1, &usage_multiply_group);
    enter_group_in_cell(&board, TEST_THIRD_GROUP_CELL, &usage_second_add_group);
    focus_slot(&board, TEST_FIRST_SLOT);
    press_digit(&board, TEST_GIVEN_ONE_VALUE);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_TWICE);
    ASSERT_EQ_INT(board.slot_usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
}

static void test_apply_board_input_clearing_an_operation_lowers_the_marks(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_group_by_inputs(&board, &usage_add_group);
    enter_group_in_cell(&board, 1, &usage_multiply_group);
    focus_slot(&board, TEST_FIRST_SLOT);
    press_digit(&board, 2);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_TWICE);
    ASSERT_EQ_INT(board.slot_usages[TEST_FIRST_SLOT], SLOT_USAGE_TWICE);
    place_group_focus(&board, 1, GROUP_FIELD_OPERATION);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(board.slot_usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
}

static void
test_apply_board_input_clearing_an_operand_box_lowers_the_marks(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_group_by_inputs(&board, &usage_add_group);
    focus_slot(&board, TEST_FIRST_SLOT);
    press_digit(&board, 2);
    place_group_focus(&board, 0, GROUP_FIELD_SMALLER);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_NONE);
    ASSERT_EQ_INT(board.slot_usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
    place_group_focus(&board, 0, GROUP_FIELD_GREATER);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_TRUE(all_usages_are(&board, SLOT_USAGE_NONE));
}

static void test_apply_board_input_changing_an_operation_moves_the_mark(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_group_by_inputs(&board, &usage_add_group);
    enter_group_in_cell(&board, 1, &usage_add_group);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_ONCE);
    place_group_focus(&board, 1, GROUP_FIELD_OPERATION);
    press_operation(&board, GROUP_OPERATION_MULTIPLY);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_TWICE);
}

static void test_apply_board_input_changing_a_group_number_moves_the_mark(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_UP);
    enter_group_by_inputs(&board, &usage_add_group);
    press(&board, BOARD_INPUT_DELETE);
    press_digit(&board, usage_second_add_group.greater);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_ONE_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(board.slot_usages[TEST_GIVEN_THREE_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(marked_usage_count(&board), 2);
}

static void test_apply_board_input_wrong_result_leaves_marks_unchanged(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_solution_groups(&board);
    fill_slots(&board, fixture_wrong_last_values);
    SlotUsage before[TETONOR_CELL_COUNT];
    memcpy(before, board.slot_usages, sizeof before);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    ASSERT_TRUE(memcmp(before, board.slot_usages, sizeof before) == 0);
    ASSERT_EQ_INT(board.slot_usages[TEST_LAST_SLOT], SLOT_USAGE_NONE);
    ASSERT_EQ_INT(board.slot_usages[0], SLOT_USAGE_TWICE);
    ASSERT_EQ_INT(marked_usage_count(&board), TETONOR_CELL_COUNT - 1);
}

static void test_apply_board_input_correct_result_leaves_marks_unchanged(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_solution_groups(&board);
    fill_slots(&board, fixture_hidden_values);
    ASSERT_TRUE(all_usages_are(&board, SLOT_USAGE_TWICE));
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(all_usages_are(&board, SLOT_USAGE_TWICE));
}

static void test_apply_board_input_recomputes_marks_after_an_ignored_input(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_solution_groups(&board);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (board.slots[i].editable)
        {
            board.slots[i].value = fixture_strip[i];
        }
    }
    ASSERT_TRUE(all_usages_are(&board, SLOT_USAGE_NONE));
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_TRUE(all_usages_are(&board, SLOT_USAGE_TWICE));
}

static void test_apply_board_input_recomputes_marks_after_every_input(void)
{
    for (int f = 0; f < TEST_FOCUS_COUNT; f++)
    {
        for (int k = 0; k < TEST_INPUT_KIND_COUNT; k++)
        {
            NumberGeneration puzzle;
            Board board;
            start_default_board(&puzzle, &board);
            place_group_focus(&board, 0, GROUP_FIELD_OPERATION);
            board.focus = fixture_focuses[f];
            set_all_usages(&board, SLOT_USAGE_TWICE);
            BoardInput input = {.kind      = fixture_input_kinds[k],
                                .digit     = TEST_UNUSED_VALUE,
                                .operation = GROUP_OPERATION_ADD};
            apply_board_input(&board, input);
            ASSERT_TRUE(all_usages_are(&board, SLOT_USAGE_NONE));
        }
    }
}

#define TEST_NOOP_INPUT_COUNT 5
#define TEST_TYPED_FIRST_VALUE 2

static const BoardInput fixture_restart_noop_inputs[TEST_NOOP_INPUT_COUNT] = {
    {.kind = BOARD_INPUT_MOVE_RIGHT},
    {.kind = BOARD_INPUT_MOVE_DOWN},
    {.kind = BOARD_INPUT_DIGIT, .digit = TEST_UNUSED_VALUE},
    {.kind = BOARD_INPUT_DELETE},
    {.kind = BOARD_INPUT_OPERATION, .operation = GROUP_OPERATION_ADD}};

static void focus_restart_by_inputs(Board* board)
{
    press(board, BOARD_INPUT_MOVE_DOWN);
    press(board, BOARD_INPUT_MOVE_DOWN);
}

/* Leaves the focus on the restart control. Either submits the given values
 * (judged) or leaves a typed slot and a typed group box unjudged. */
static void start_restart_board(NumberGeneration* puzzle, Board* board,
                                const int values[TEST_EDITABLE_COUNT],
                                bool judged)
{
    start_default_board(puzzle, board);
    if (judged)
    {
        fill_and_submit(board, values);
    }
    else
    {
        type_value(board, TEST_TYPED_FIRST_VALUE);
        board->groups[0].smaller = 1;
        board->focus             = BOARD_FOCUS_SUBMIT;
    }
    press(board, BOARD_INPUT_MOVE_DOWN);
}

static void test_apply_board_input_move_up_on_restart_focuses_submit(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 6, GROUP_FIELD_OPERATION);
    board.cursor     = TEST_SECOND_SLOT;
    board.focus      = BOARD_FOCUS_RESTART;
    Board before     = board;
    BoardEvent event = press_for_event(&board, BOARD_INPUT_MOVE_UP);
    ASSERT_EQ_INT(event, BOARD_EVENT_NONE);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
    ASSERT_TRUE(equal_apart_from_focus(&board, &before));
}

static void test_apply_board_input_activate_on_restart_requests_restart(void)
{
    NumberGeneration puzzle;
    Board board;
    start_restart_board(&puzzle, &board, fixture_hidden_values, false);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_RESTART);
    Board before     = board;
    BoardEvent event = press_for_event(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(event, BOARD_EVENT_RESTART);
    ASSERT_TRUE(boards_equal(&board, &before));
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void
test_apply_board_input_activate_on_restart_after_correct_requests_restart(void)
{
    NumberGeneration puzzle;
    Board board;
    start_restart_board(&puzzle, &board, fixture_hidden_values, true);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_RESTART);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    Board before     = board;
    BoardEvent event = press_for_event(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(event, BOARD_EVENT_RESTART);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void
test_apply_board_input_activate_on_restart_after_wrong_requests_restart(void)
{
    NumberGeneration puzzle;
    Board board;
    start_restart_board(&puzzle, &board, fixture_wrong_last_values, true);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_RESTART);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    Board before     = board;
    BoardEvent event = press_for_event(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(event, BOARD_EVENT_RESTART);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void test_apply_board_input_activate_on_restart_does_not_judge(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_solution_groups(&board);
    for (int i = 0; i < TEST_EDITABLE_COUNT; i++)
    {
        board.slots[fixture_hidden[i]].value = fixture_hidden_values[i];
    }
    board.focus      = BOARD_FOCUS_RESTART;
    BoardEvent event = press_for_event(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(event, BOARD_EVENT_RESTART);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void assert_noop_inputs_on_restart(Board* board)
{
    BoardResult result = board->result;
    for (int i = 0; i < TEST_NOOP_INPUT_COUNT; i++)
    {
        Board before = *board;
        BoardEvent event =
            apply_for_event(board, fixture_restart_noop_inputs[i]);
        ASSERT_EQ_INT(event, BOARD_EVENT_NONE);
        ASSERT_EQ_INT(board->focus, BOARD_FOCUS_RESTART);
        ASSERT_EQ_INT(board->result, result);
        ASSERT_TRUE(boards_equal(board, &before));
    }
}

static void test_apply_board_input_noop_inputs_on_restart(void)
{
    NumberGeneration pending_puzzle;
    Board pending;
    start_restart_board(&pending_puzzle, &pending, fixture_hidden_values,
                        false);
    assert_noop_inputs_on_restart(&pending);
    NumberGeneration wrong_puzzle;
    Board wrong;
    start_restart_board(&wrong_puzzle, &wrong, fixture_wrong_last_values, true);
    ASSERT_EQ_INT(wrong.result, BOARD_RESULT_WRONG);
    assert_noop_inputs_on_restart(&wrong);
    NumberGeneration correct_puzzle;
    Board correct;
    start_restart_board(&correct_puzzle, &correct, fixture_hidden_values, true);
    ASSERT_EQ_INT(correct.result, BOARD_RESULT_CORRECT);
    assert_noop_inputs_on_restart(&correct);
}

static void test_apply_board_input_move_left_on_restart_returns_to_slot(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    focus_restart_by_inputs(&board);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_RESTART);
    Board before     = board;
    BoardEvent event = press_for_event(&board, BOARD_INPUT_MOVE_LEFT);
    ASSERT_EQ_INT(event, BOARD_EVENT_NONE);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
    ASSERT_EQ_INT(board.cursor, TEST_SECOND_SLOT);
    ASSERT_TRUE(equal_apart_from_focus(&board, &before));
}

static void
test_apply_board_input_without_editable_slots_left_on_restart_focuses_group(
    void)
{
    NumberGeneration puzzle;
    Board board;
    start_board_without_slots(&puzzle, &board);
    place_group_focus(&board, TEST_LAST_CELL - 1, GROUP_FIELD_OPERATION);
    board.focus = BOARD_FOCUS_SUBMIT;
    press(&board, BOARD_INPUT_MOVE_DOWN);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_RESTART);
    Board before     = board;
    BoardEvent event = press_for_event(&board, BOARD_INPUT_MOVE_LEFT);
    ASSERT_EQ_INT(event, BOARD_EVENT_NONE);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    ASSERT_EQ_INT(board.cursor, TETONOR_NO_CURSOR);
    ASSERT_EQ_INT(board.group_cursor, TEST_LAST_CELL - 1);
    ASSERT_EQ_INT(board.group_field, GROUP_FIELD_OPERATION);
    ASSERT_TRUE(equal_apart_from_focus(&board, &before));
}

static void test_apply_board_input_returns_none_outside_restart(void)
{
    for (int f = 0; f < TEST_FOCUS_COUNT; f++)
    {
        if (fixture_focuses[f] == BOARD_FOCUS_RESTART)
        {
            continue;
        }
        for (int k = 0; k < TEST_INPUT_KIND_COUNT; k++)
        {
            NumberGeneration puzzle;
            Board board;
            start_default_board(&puzzle, &board);
            fill_solution_groups(&board);
            for (int i = 0; i < TEST_EDITABLE_COUNT; i++)
            {
                board.slots[fixture_hidden[i]].value = fixture_hidden_values[i];
            }
            place_group_focus(&board, 0, GROUP_FIELD_OPERATION);
            board.focus      = fixture_focuses[f];
            BoardInput input = {.kind      = fixture_input_kinds[k],
                                .digit     = TEST_UNUSED_VALUE,
                                .operation = GROUP_OPERATION_ADD};
            ASSERT_EQ_INT(apply_for_event(&board, input), BOARD_EVENT_NONE);
        }
    }
}

static void
test_apply_board_input_activate_on_submit_judges_and_returns_none(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_solution_groups(&board);
    for (int i = 0; i < TEST_EDITABLE_COUNT; i++)
    {
        board.slots[fixture_hidden[i]].value = fixture_hidden_values[i];
    }
    board.focus      = BOARD_FOCUS_SUBMIT;
    BoardEvent event = press_for_event(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(event, BOARD_EVENT_NONE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void test_apply_board_input_slot_down_down_reaches_restart(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
    ASSERT_EQ_INT(press_for_event(&board, BOARD_INPUT_MOVE_DOWN),
                  BOARD_EVENT_NONE);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
    ASSERT_EQ_INT(press_for_event(&board, BOARD_INPUT_MOVE_DOWN),
                  BOARD_EVENT_NONE);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_RESTART);
}

static void test_apply_board_input_restart_up_up_returns_to_slot(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    focus_restart_by_inputs(&board);
    press(&board, BOARD_INPUT_MOVE_UP);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
    press(&board, BOARD_INPUT_MOVE_UP);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
    ASSERT_EQ_INT(board.cursor, TEST_FIRST_SLOT);
}

#define TEST_UNDO_LOOP_LIMIT 100
#define TEST_FILL_MOVE_COUNT 5 /* digits of 2, 5, 6 and 28 */
#define TEST_OVERFLOW_MOVES (TETONOR_UNDO_HISTORY_DEPTH + 1)
#define TEST_WRAP_MOVES 30
#define TEST_REPEATED_WRAP_MOVES 50
#define TEST_MIXED_MOVE_COUNT 5
#define TEST_OTHER_DIGIT 3
#define TEST_OVER_MAXIMUM_DIGIT 5

/* Everything a move can change; focus, cursors, palette and history are left
 * out on purpose. */
static bool entries_equal(const Board* first, const Board* second)
{
    return slots_equal(first, second) &&
           groups_equal(first->groups, second->groups) &&
           first->result == second->result &&
           memcmp(first->slot_usages, second->slot_usages,
                  sizeof first->slot_usages) == 0;
}

static bool positions_equal(const Board* first, const Board* second)
{
    return first->cursor == second->cursor &&
           first->group_cursor == second->group_cursor &&
           first->group_field == second->group_field &&
           first->focus == second->focus;
}

/* Bounded, so that a history that never runs out cannot hang the test. */
static int count_undos(Board* board)
{
    int undone = 0;
    while (undone < TEST_UNDO_LOOP_LIMIT && undo_board_move(board))
    {
        undone++;
    }
    return undone;
}

/* Types one digit into the index-th group number box, counting the smaller and
 * the greater box of cell 0, then of cell 1, and so on. Every box starts empty,
 * so each call changes the board; 32 boxes allow 32 distinct moves. */
static void make_group_move(Board* board, int index)
{
    place_group_focus(board, index / TEST_NUMBER_FIELD_COUNT,
                      fixture_number_fields[index % TEST_NUMBER_FIELD_COUNT]);
    press_digit(board, index % TETONOR_DIGIT_BASE);
}

/* Alternates a typed digit and its deletion in the first editable slot, so
 * that any number of moves can be made. */
static void make_slot_edit(Board* board, int index)
{
    focus_slot(board, TEST_FIRST_SLOT);
    if (index % 2 == 0)
    {
        press_digit(board, 1 + index % (TETONOR_DIGIT_BASE - 1));
        return;
    }
    press(board, BOARD_INPUT_DELETE);
}

/* Every movement from every focus, and ACTIVATE on each focus of an incomplete
 * board. */
static void press_ignored_movements(Board* board)
{
    for (int f = 0; f < TEST_FOCUS_COUNT; f++)
    {
        for (int k = 0; k < TEST_MOVE_KIND_COUNT; k++)
        {
            board->focus = fixture_focuses[f];
            press(board, fixture_move_kinds[k]);
        }
        board->focus = fixture_focuses[f];
        press(board, BOARD_INPUT_ACTIVATE);
    }
}

/* Edits that change nothing. Needs the second slot and the last group cell to
 * be empty. */
static void press_ignored_edits(Board* board)
{
    focus_slot(board, TEST_SECOND_SLOT);
    press_digit(board, TEST_OUT_OF_RANGE_DIGIT);
    press_digit(board, -1);
    press(board, BOARD_INPUT_DELETE);
    press_operation(board, GROUP_OPERATION_ADD);
    place_group_focus(board, TEST_LAST_CELL, GROUP_FIELD_SMALLER);
    press_digit(board, TEST_OUT_OF_RANGE_DIGIT);
    press_digit(board, -1);
    press(board, BOARD_INPUT_DELETE);
    press_operation(board, GROUP_OPERATION_ADD);
    place_group_focus(board, TEST_LAST_CELL, GROUP_FIELD_OPERATION);
    press(board, BOARD_INPUT_DELETE);
    press_operation(board, GROUP_OPERATION_NONE);
    press_digit(board, TEST_OVER_MAXIMUM_DIGIT);
}

static void test_undo_board_move_history_depth_is_twenty(void)
{
    ASSERT_EQ_INT(TETONOR_UNDO_HISTORY_DEPTH, 20);
}

static void test_undo_board_move_on_fresh_board_reports_nothing(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board before = board;
    ASSERT_FALSE(undo_board_move(&board));
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void
test_undo_board_move_on_fresh_board_without_slots_reports_nothing(void)
{
    NumberGeneration puzzle;
    Board board;
    start_board_without_slots(&puzzle, &board);
    Board before = board;
    ASSERT_FALSE(undo_board_move(&board));
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void test_undo_board_move_on_empty_history_leaves_usages_untouched(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    set_all_usages(&board, SLOT_USAGE_TWICE);
    ASSERT_FALSE(undo_board_move(&board));
    ASSERT_TRUE(all_usages_are(&board, SLOT_USAGE_TWICE));
}

static void test_undo_board_move_on_exhausted_history_reports_nothing(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 7);
    ASSERT_TRUE(undo_board_move(&board));
    Board before = board;
    ASSERT_FALSE(undo_board_move(&board));
    ASSERT_FALSE(undo_board_move(&board));
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void test_undo_board_move_reverts_typed_digit_in_slot(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board empty = board;
    press_digit(&board, 7);
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TETONOR_EMPTY_ENTRY);
    ASSERT_TRUE(entries_equal(&board, &empty));
    press_digit(&board, 1);
    Board one = board;
    press_digit(&board, 2);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 12);
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 1);
    ASSERT_TRUE(entries_equal(&board, &one));
}

static void test_undo_board_move_reverts_typed_digit_in_group_box(void)
{
    for (int f = 0; f < TEST_NUMBER_FIELD_COUNT; f++)
    {
        NumberGeneration puzzle;
        Board board;
        start_default_board(&puzzle, &board);
        place_group_focus(&board, 6, fixture_number_fields[f]);
        Board empty = board;
        press_digit(&board, 7);
        ASSERT_FALSE(all_groups_empty(&board));
        ASSERT_TRUE(undo_board_move(&board));
        ASSERT_TRUE(all_groups_empty(&board));
        ASSERT_TRUE(entries_equal(&board, &empty));
        press_digit(&board, 1);
        Board one = board;
        press_digit(&board, 2);
        ASSERT_EQ_INT(group_number(&board, 6, fixture_number_fields[f]), 12);
        ASSERT_TRUE(undo_board_move(&board));
        ASSERT_EQ_INT(group_number(&board, 6, fixture_number_fields[f]), 1);
        ASSERT_TRUE(entries_equal(&board, &one));
    }
}

static void test_undo_board_move_reverts_replaced_leading_zero_to_zero(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 0);
    press_digit(&board, 7);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 7);
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 0);
}

static void test_undo_board_move_reverts_deleted_digit_in_slot(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    type_value(&board, 12);
    Board typed = board;
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 1);
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 12);
    ASSERT_TRUE(entries_equal(&board, &typed));
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_TRUE(undo_board_move(&board));
    press_digit(&board, 7);
    typed = board;
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TETONOR_EMPTY_ENTRY);
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 7);
    ASSERT_TRUE(entries_equal(&board, &typed));
}

static void test_undo_board_move_reverts_deleted_digit_in_group_box(void)
{
    for (int f = 0; f < TEST_NUMBER_FIELD_COUNT; f++)
    {
        NumberGeneration puzzle;
        Board board;
        start_default_board(&puzzle, &board);
        place_group_focus(&board, 9, fixture_number_fields[f]);
        type_value(&board, 12);
        Board typed = board;
        press(&board, BOARD_INPUT_DELETE);
        ASSERT_EQ_INT(group_number(&board, 9, fixture_number_fields[f]), 1);
        ASSERT_TRUE(undo_board_move(&board));
        ASSERT_EQ_INT(group_number(&board, 9, fixture_number_fields[f]), 12);
        ASSERT_TRUE(entries_equal(&board, &typed));
        ASSERT_TRUE(undo_board_move(&board));
        ASSERT_TRUE(undo_board_move(&board));
        press_digit(&board, 7);
        typed = board;
        press(&board, BOARD_INPUT_DELETE);
        ASSERT_TRUE(all_groups_empty(&board));
        ASSERT_TRUE(undo_board_move(&board));
        ASSERT_EQ_INT(group_number(&board, 9, fixture_number_fields[f]), 7);
        ASSERT_TRUE(entries_equal(&board, &typed));
    }
}

static void test_undo_board_move_reverts_operation(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 3, GROUP_FIELD_OPERATION);
    Board none = board;
    press_operation(&board, GROUP_OPERATION_ADD);
    ASSERT_EQ_INT(board.groups[3].operation, GROUP_OPERATION_ADD);
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_EQ_INT(board.groups[3].operation, GROUP_OPERATION_NONE);
    ASSERT_TRUE(entries_equal(&board, &none));

    press_operation(&board, GROUP_OPERATION_ADD);
    Board add = board;
    press_operation(&board, GROUP_OPERATION_MULTIPLY);
    ASSERT_EQ_INT(board.groups[3].operation, GROUP_OPERATION_MULTIPLY);
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_EQ_INT(board.groups[3].operation, GROUP_OPERATION_ADD);
    ASSERT_TRUE(entries_equal(&board, &add));

    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.groups[3].operation, GROUP_OPERATION_NONE);
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_EQ_INT(board.groups[3].operation, GROUP_OPERATION_ADD);
    ASSERT_TRUE(entries_equal(&board, &add));
}

static void test_undo_board_move_recomputes_slot_usages(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board initial = board;
    press(&board, BOARD_INPUT_MOVE_UP);
    type_value(&board, usage_add_group.smaller);
    Board after_smaller = board;
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    press_operation(&board, usage_add_group.operation);
    Board after_operation = board;
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    type_value(&board, usage_add_group.greater);
    ASSERT_EQ_INT(after_operation.slot_usages[TEST_GIVEN_ONE_SLOT],
                  SLOT_USAGE_ONCE);
    ASSERT_TRUE(all_usages_are(&after_smaller, SLOT_USAGE_NONE));

    set_all_usages(&board, SLOT_USAGE_TWICE);
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_TRUE(entries_equal(&board, &after_operation));
    ASSERT_EQ_INT(marked_usage_count(&board), 1);

    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_TRUE(entries_equal(&board, &after_smaller));
    ASSERT_TRUE(all_usages_are(&board, SLOT_USAGE_NONE));

    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_TRUE(entries_equal(&board, &initial));
}

static void test_undo_board_move_restores_exact_entries_of_each_move(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board before_digit = board;
    press_digit(&board, 7);
    Board before_delete = board;
    press(&board, BOARD_INPUT_DELETE);
    place_group_focus(&board, 0, GROUP_FIELD_OPERATION);
    Board before_operation = board;
    press_operation(&board, GROUP_OPERATION_MULTIPLY);

    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_TRUE(entries_equal(&board, &before_operation));
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_TRUE(entries_equal(&board, &before_delete));
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_TRUE(entries_equal(&board, &before_digit));
    ASSERT_FALSE(undo_board_move(&board));
}

static void test_undo_board_move_keeps_cursor_and_focus(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 7);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    press(&board, BOARD_INPUT_MOVE_UP);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    Board positioned = board;
    ASSERT_EQ_INT(board.cursor, TEST_SECOND_SLOT);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    ASSERT_EQ_INT(board.group_cursor, TETONOR_GRID_SIZE);
    ASSERT_EQ_INT(board.group_field, GROUP_FIELD_GREATER);

    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_TRUE(positions_equal(&board, &positioned));
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TETONOR_EMPTY_ENTRY);
}

static void test_undo_board_move_works_from_every_focus(void)
{
    for (int f = 0; f < TEST_FOCUS_COUNT; f++)
    {
        NumberGeneration puzzle;
        Board board;
        start_default_board(&puzzle, &board);
        Board empty = board;
        press_digit(&board, 7);
        board.focus  = fixture_focuses[f];
        Board before = board;
        ASSERT_TRUE(undo_board_move(&board));
        ASSERT_TRUE(positions_equal(&board, &before));
        ASSERT_TRUE(entries_equal(&board, &empty));
        ASSERT_FALSE(undo_board_move(&board));
    }
}

static void test_undo_board_move_reverts_moves_in_reverse_order(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board states[TEST_MIXED_MOVE_COUNT + 1];
    states[0] = board;
    press_digit(&board, 7);
    states[1] = board;
    place_group_focus(&board, 0, GROUP_FIELD_SMALLER);
    press_digit(&board, 4);
    states[2] = board;
    place_group_focus(&board, 0, GROUP_FIELD_OPERATION);
    press_operation(&board, GROUP_OPERATION_MULTIPLY);
    states[3] = board;
    place_group_focus(&board, 0, GROUP_FIELD_GREATER);
    press_digit(&board, 5);
    states[4] = board;
    focus_slot(&board, TEST_FIRST_SLOT);
    press(&board, BOARD_INPUT_DELETE);
    states[5] = board;

    for (int undone = 1; undone <= TEST_MIXED_MOVE_COUNT; undone++)
    {
        ASSERT_TRUE(undo_board_move(&board));
        ASSERT_TRUE(
            entries_equal(&board, &states[TEST_MIXED_MOVE_COUNT - undone]));
    }
    ASSERT_FALSE(undo_board_move(&board));
}

static void test_undo_board_move_keeps_only_the_last_twenty_moves(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board states[TEST_OVERFLOW_MOVES + 1];
    states[0] = board;
    for (int move = 0; move < TEST_OVERFLOW_MOVES; move++)
    {
        make_group_move(&board, move);
        states[move + 1] = board;
    }
    for (int undone = 1; undone <= TETONOR_UNDO_HISTORY_DEPTH; undone++)
    {
        ASSERT_TRUE(undo_board_move(&board));
        ASSERT_TRUE(
            entries_equal(&board, &states[TEST_OVERFLOW_MOVES - undone]));
    }
    ASSERT_TRUE(entries_equal(&board, &states[1]));
    Board before = board;
    ASSERT_FALSE(undo_board_move(&board));
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void
test_undo_board_move_keeps_exactly_twenty_moves_when_twenty_made(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board empty = board;
    for (int move = 0; move < TETONOR_UNDO_HISTORY_DEPTH; move++)
    {
        make_group_move(&board, move);
    }
    ASSERT_EQ_INT(count_undos(&board), TETONOR_UNDO_HISTORY_DEPTH);
    ASSERT_TRUE(entries_equal(&board, &empty));
}

static void test_undo_board_move_wraps_after_many_moves(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board expected = board;
    for (int move = 0; move < TEST_WRAP_MOVES; move++)
    {
        if (move == TEST_WRAP_MOVES - TETONOR_UNDO_HISTORY_DEPTH)
        {
            expected = board;
        }
        make_group_move(&board, move);
    }
    ASSERT_EQ_INT(count_undos(&board), TETONOR_UNDO_HISTORY_DEPTH);
    ASSERT_TRUE(entries_equal(&board, &expected));
}

static void test_undo_board_move_survives_repeated_ring_wraparound(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board expected = board;
    for (int move = 0; move < TEST_REPEATED_WRAP_MOVES; move++)
    {
        if (move == TEST_REPEATED_WRAP_MOVES - TETONOR_UNDO_HISTORY_DEPTH)
        {
            expected = board;
        }
        make_slot_edit(&board, move);
    }
    ASSERT_EQ_INT(count_undos(&board), TETONOR_UNDO_HISTORY_DEPTH);
    ASSERT_TRUE(entries_equal(&board, &expected));
}

static void test_undo_board_move_then_new_move_is_undoable(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board states[4];
    states[0] = board;
    for (int move = 0; move < 3; move++)
    {
        make_group_move(&board, move);
        states[move + 1] = board;
    }
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_TRUE(entries_equal(&board, &states[2]));
    make_group_move(&board, 10);
    ASSERT_FALSE(entries_equal(&board, &states[2]));
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_TRUE(entries_equal(&board, &states[2]));
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_TRUE(entries_equal(&board, &states[1]));
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_TRUE(entries_equal(&board, &states[0]));
    ASSERT_FALSE(undo_board_move(&board));
}

static void test_apply_board_input_noop_inputs_add_no_history(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board empty = board;
    press_digit(&board, TEST_OTHER_DIGIT);
    press_digit(&board, TEST_OVER_MAXIMUM_DIGIT);
    press_digit(&board, 0);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TEST_OTHER_DIGIT);
    press_ignored_movements(&board);
    press_ignored_edits(&board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_EQ_INT(count_undos(&board), 1);
    ASSERT_TRUE(entries_equal(&board, &empty));
}

static void test_apply_board_input_noop_inputs_do_not_evict_history(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board empty = board;
    for (int move = 0; move < TETONOR_UNDO_HISTORY_DEPTH; move++)
    {
        make_group_move(&board, move);
    }
    place_group_focus(&board, 1, GROUP_FIELD_GREATER);
    ASSERT_EQ_INT(board.groups[1].greater, TEST_OTHER_DIGIT);
    press_digit(&board, TEST_OVER_MAXIMUM_DIGIT);
    press_digit(&board, 0);
    ASSERT_EQ_INT(board.groups[1].greater, TEST_OTHER_DIGIT);
    press_ignored_movements(&board);
    press_ignored_edits(&board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_EQ_INT(count_undos(&board), TETONOR_UNDO_HISTORY_DEPTH);
    ASSERT_TRUE(entries_equal(&board, &empty));
}

static void
test_apply_board_input_setting_the_same_operation_is_not_a_move(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 3, GROUP_FIELD_OPERATION);
    Board none = board;
    press_operation(&board, GROUP_OPERATION_ADD);
    press_operation(&board, GROUP_OPERATION_ADD);
    ASSERT_EQ_INT(count_undos(&board), 1);
    ASSERT_TRUE(entries_equal(&board, &none));
}

static void test_apply_board_input_repeated_zero_is_not_a_move(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board empty = board;
    press_digit(&board, 0);
    press_digit(&board, 0);
    ASSERT_EQ_INT(count_undos(&board), 1);
    ASSERT_TRUE(entries_equal(&board, &empty));
}

static void test_apply_board_input_confirm_is_not_a_move(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_and_submit(&board, fixture_wrong_last_values);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_EQ_INT(board.slots[TEST_LAST_SLOT].value, 2);
    ASSERT_EQ_INT(board.slots[TEST_THIRD_SLOT].value, 6);
    ASSERT_TRUE(groups_equal(board.groups, fixture_groups));
    ASSERT_EQ_INT(count_undos(&board), TEST_FILL_MOVE_COUNT - 1);
}

static void assert_undo_restores_result(const int values[TEST_EDITABLE_COUNT],
                                        BoardResult expected)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_and_submit(&board, values);
    ASSERT_EQ_INT(board.result, expected);
    Board judged = board;
    focus_slot(&board, TEST_LAST_SLOT);
    press_digit(&board, TEST_OTHER_DIGIT);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_EQ_INT(board.slots[TEST_LAST_SLOT].value, TEST_OTHER_DIGIT);
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_EQ_INT(board.result, expected);
    ASSERT_TRUE(entries_equal(&board, &judged));
}

static void test_undo_board_move_restores_result_before_the_edit(void)
{
    assert_undo_restores_result(fixture_wrong_last_values, BOARD_RESULT_WRONG);

    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    Board judged = board;
    int moves    = board.history.count;
    focus_slot(&board, TEST_LAST_SLOT);
    press_digit(&board, TEST_OTHER_DIGIT);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(entries_equal(&board, &judged));
    ASSERT_FALSE(undo_board_move(&board));
    ASSERT_EQ_INT(board.history.count, moves);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(entries_equal(&board, &judged));
}

static void test_apply_board_input_result_only_change_is_not_a_move(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_and_submit(&board, fixture_wrong_last_values);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    focus_slot(&board, TEST_FIRST_SLOT);
    Board judged = board;
    press_digit(&board, fixture_wrong_last_values[0]);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_TRUE(slots_equal(&board, &judged));
    ASSERT_EQ_INT(count_undos(&board), TEST_FILL_MOVE_COUNT);
}

static void test_init_board_empties_move_history(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    for (int move = 0; move < TEST_MIXED_MOVE_COUNT; move++)
    {
        make_group_move(&board, move);
    }
    init_board(&board, &puzzle);
    Board fresh = board;
    ASSERT_FALSE(undo_board_move(&board));
    ASSERT_TRUE(boards_equal(&board, &fresh));
    ASSERT_TRUE(all_groups_empty(&board));
}

static void test_init_board_empties_a_full_move_history(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    for (int move = 0; move < TEST_WRAP_MOVES; move++)
    {
        make_group_move(&board, move);
    }
    init_board(&board, &puzzle);
    ASSERT_EQ_INT(count_undos(&board), 0);
    press_digit(&board, 7);
    ASSERT_EQ_INT(count_undos(&board), 1);
}

static void test_undo_board_move_without_editable_slots(void)
{
    NumberGeneration puzzle;
    Board board;
    start_board_without_slots(&puzzle, &board);
    press_digit(&board, 5);
    ASSERT_EQ_INT(board.groups[0].smaller, 5);
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_TRUE(all_groups_empty(&board));
    ASSERT_EQ_INT(board.cursor, TETONOR_NO_CURSOR);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    ASSERT_FALSE(undo_board_move(&board));
}

static void test_undo_board_move_keeps_puzzle_palette_and_editable_flags(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board initial = board;
    press_digit(&board, 7);
    board.palette.foregrounds[PALETTE_ROLE_USED_ONCE].color =
        PALETTE_COLOR_MAGENTA;
    Board before = board;
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_TRUE(board.puzzle == &puzzle);
    ASSERT_FALSE(palette_is_default(&board.palette));
    ASSERT_TRUE(palette_entries_equal(
        &board.palette.foregrounds[PALETTE_ROLE_USED_ONCE],
        &before.palette.foregrounds[PALETTE_ROLE_USED_ONCE]));
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        ASSERT_EQ_INT(board.slots[i].editable, initial.slots[i].editable);
    }
}

#define TEST_LOCKED_FOCUS_COUNT 6
#define TEST_LOCKED_CELL 3
#define TEST_SEQUENCE_START_COUNT 2
#define TEST_ELEMENT_GROUP_GREATER 2
#define TEST_ELEMENT_SUBMIT 4
#define TEST_BOX_ELEMENT_COUNT 4

/* Puts the focus on one kind of element: 0 slot, 1 group smaller box, 2 group
 * greater box, 3 operation box, 4 submit control, 5 restart control. */
static void focus_element(Board* board, int element)
{
    switch (element)
    {
        case 0:
            focus_slot(board, TEST_LAST_SLOT);
            break;
        case 1:
            place_group_focus(board, TEST_LOCKED_CELL, GROUP_FIELD_SMALLER);
            break;
        case 2:
            place_group_focus(board, TEST_LOCKED_CELL, GROUP_FIELD_GREATER);
            break;
        case 3:
            place_group_focus(board, TEST_LOCKED_CELL, GROUP_FIELD_OPERATION);
            break;
        case 4:
            board->focus = BOARD_FOCUS_SUBMIT;
            break;
        default:
            board->focus = BOARD_FOCUS_RESTART;
            break;
    }
}

/* Every field an input could change, the move history included. */
static bool locked_board_unchanged(const Board* board, const Board* before)
{
    return boards_equal(board, before) &&
           board->history.count == before->history.count &&
           board->history.newest == before->history.newest &&
           board->result == BOARD_RESULT_CORRECT;
}

static void assert_locked_board_ignores(Board* board, BoardInput input)
{
    Board before     = *board;
    BoardEvent event = apply_board_input(board, input);
    ASSERT_EQ_INT(event, BOARD_EVENT_NONE);
    ASSERT_TRUE(locked_board_unchanged(board, &before));
}

static void test_is_board_locked_follows_the_result(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    ASSERT_FALSE(is_board_locked(&board));
    press_digit(&board, 7);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_FALSE(is_board_locked(&board));

    start_wrong_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    ASSERT_FALSE(is_board_locked(&board));

    start_correct_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(is_board_locked(&board));
    press(&board, BOARD_INPUT_MOVE_UP);
    ASSERT_TRUE(is_board_locked(&board));

    init_board(&board, &puzzle);
    ASSERT_FALSE(is_board_locked(&board));
}

static void test_apply_board_input_locked_board_ignores_digits(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    ASSERT_TRUE(board.history.count > 0);
    for (int element = 0; element < TEST_LOCKED_FOCUS_COUNT; element++)
    {
        focus_element(&board, element);
        for (int digit = 0; digit < TETONOR_DIGIT_BASE; digit++)
        {
            BoardInput input = {.kind = BOARD_INPUT_DIGIT, .digit = digit};
            assert_locked_board_ignores(&board, input);
        }
    }
    ASSERT_EQ_INT(board.slots[TEST_LAST_SLOT].value, 29);
    ASSERT_TRUE(groups_equal(board.groups, fixture_groups));
}

static void test_apply_board_input_locked_board_ignores_operations(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    const GroupOperation operations[] = {
        GROUP_OPERATION_ADD, GROUP_OPERATION_MULTIPLY, GROUP_OPERATION_NONE};
    for (int element = 0; element < TEST_LOCKED_FOCUS_COUNT; element++)
    {
        focus_element(&board, element);
        for (size_t i = 0; i < sizeof operations / sizeof operations[0]; i++)
        {
            BoardInput input = {.kind      = BOARD_INPUT_OPERATION,
                                .operation = operations[i]};
            assert_locked_board_ignores(&board, input);
        }
    }
    ASSERT_TRUE(groups_equal(board.groups, fixture_groups));
}

static void test_apply_board_input_locked_board_ignores_delete(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    BoardInput input = {.kind = BOARD_INPUT_DELETE};
    for (int element = 0; element < TEST_LOCKED_FOCUS_COUNT; element++)
    {
        focus_element(&board, element);
        assert_locked_board_ignores(&board, input);
        assert_locked_board_ignores(&board, input);
        assert_locked_board_ignores(&board, input);
    }
    ASSERT_EQ_INT(board.slots[TEST_LAST_SLOT].value, 29);
    ASSERT_TRUE(groups_equal(board.groups, fixture_groups));
}

static void test_apply_board_input_locked_board_ignores_confirm(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    board.focus      = BOARD_FOCUS_SUBMIT;
    BoardInput input = {.kind = BOARD_INPUT_ACTIVATE};
    assert_locked_board_ignores(&board, input);

    /* Were CONFIRM allowed, judging this entry again would give WRONG. */
    board.slots[TEST_LAST_SLOT].value = TEST_WRONG_VALUE;
    assert_locked_board_ignores(&board, input);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void test_apply_board_input_locked_board_activating_a_box_is_a_noop(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    BoardInput input = {.kind = BOARD_INPUT_ACTIVATE};
    for (int element = 0; element < TEST_BOX_ELEMENT_COUNT; element++)
    {
        focus_element(&board, element);
        assert_locked_board_ignores(&board, input);
    }
}

static void test_apply_board_input_locked_board_allows_movement(void)
{
    NumberGeneration puzzle;
    Board locked;
    start_correct_board(&puzzle, &locked);
    for (int start = 0; start < TEST_SEQUENCE_START_COUNT; start++)
    {
        for (int sequence = 0; sequence < TEST_MOVE_SEQUENCE_COUNT; sequence++)
        {
            Board board = locked;
            focus_element(&board, start == 0 ? TEST_ELEMENT_SUBMIT
                                             : TEST_ELEMENT_GROUP_GREATER);
            Board free_board  = board;
            free_board.result = BOARD_RESULT_PENDING;
            Board before      = board;
            press_movement_sequence(&board, sequence);
            press_movement_sequence(&free_board, sequence);
            ASSERT_EQ_INT(board.cursor, free_board.cursor);
            ASSERT_EQ_INT(board.focus, free_board.focus);
            ASSERT_EQ_INT(board.group_cursor, free_board.group_cursor);
            ASSERT_EQ_INT(board.group_field, free_board.group_field);
            ASSERT_TRUE(entries_equal(&board, &before));
            ASSERT_EQ_INT(board.history.count, before.history.count);
            ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
        }
    }
}

static void test_apply_board_input_locked_board_movement_changes_focus(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
    ASSERT_EQ_INT(press_for_event(&board, BOARD_INPUT_MOVE_UP),
                  BOARD_EVENT_NONE);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SLOT);
    press(&board, BOARD_INPUT_MOVE_UP);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_GROUP);
    press_times(&board, BOARD_INPUT_MOVE_DOWN, TETONOR_GRID_SIZE + 1);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_SUBMIT);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_RESTART);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void test_apply_board_input_locked_board_still_restarts(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    ASSERT_EQ_INT(board.focus, BOARD_FOCUS_RESTART);
    Board before = board;
    ASSERT_EQ_INT(press_for_event(&board, BOARD_INPUT_ACTIVATE),
                  BOARD_EVENT_RESTART);
    ASSERT_TRUE(locked_board_unchanged(&board, &before));
}

static void test_undo_board_move_locked_board_does_nothing(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    int moves = board.history.count;
    ASSERT_TRUE(moves > 0);
    Board before = board;
    ASSERT_FALSE(undo_board_move(&board));
    ASSERT_FALSE(undo_board_move(&board));
    ASSERT_TRUE(locked_board_unchanged(&board, &before));
    ASSERT_EQ_INT(board.history.count, moves);
    ASSERT_EQ_INT(board.slots[TEST_LAST_SLOT].value, 29);
}

static void test_undo_board_move_works_again_after_a_new_game(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    init_board(&board, &puzzle);
    press_digit(&board, 7);
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TETONOR_EMPTY_ENTRY);
}

static void test_board_progress_would_be_lost_rules(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    ASSERT_FALSE(board_progress_would_be_lost(&board));

    press(&board, BOARD_INPUT_MOVE_RIGHT);
    press(&board, BOARD_INPUT_MOVE_LEFT);
    press(&board, BOARD_INPUT_DELETE);
    press_digit(&board, TEST_OUT_OF_RANGE_DIGIT);
    press(&board, BOARD_INPUT_MOVE_DOWN);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.history.count, 0);
    ASSERT_FALSE(board_progress_would_be_lost(&board));

    press(&board, BOARD_INPUT_MOVE_UP);
    press_digit(&board, 7);
    ASSERT_TRUE(board_progress_would_be_lost(&board));
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_FALSE(board_progress_would_be_lost(&board));

    start_wrong_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    ASSERT_TRUE(board_progress_would_be_lost(&board));

    start_correct_board(&puzzle, &board);
    ASSERT_TRUE(board.history.count > 0);
    ASSERT_FALSE(board_progress_would_be_lost(&board));

    init_board(&board, &puzzle);
    ASSERT_FALSE(board_progress_would_be_lost(&board));
}

static void test_board_progress_would_be_lost_with_a_full_history(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    for (int move = 0; move < TETONOR_UNDO_HISTORY_DEPTH + 1; move++)
    {
        retype_slot(&board, move % TETONOR_DIGIT_BASE);
    }
    ASSERT_EQ_INT(board.history.count, TETONOR_UNDO_HISTORY_DEPTH);
    ASSERT_TRUE(board_progress_would_be_lost(&board));
}

#define TEST_PRESET_MOVE_COUNT 77
#define TEST_MANY_MOVES 200
#define TEST_EXTRA_MOVES 5

static void test_init_board_zeroes_move_count(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    ASSERT_EQ_INT(board.move_count, 0);
    board.move_count = TEST_PRESET_MOVE_COUNT;
    init_board(&board, &puzzle);
    ASSERT_EQ_INT(board.move_count, 0);
    press_digit(&board, TEST_OTHER_DIGIT);
    press_digit(&board, 0);
    ASSERT_TRUE(board.move_count > 0);
    init_board(&board, &puzzle);
    ASSERT_EQ_INT(board.move_count, 0);
}

static void test_apply_board_input_digit_counts_a_move(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 2);
    ASSERT_EQ_INT(board.move_count, 1);
    press_digit(&board, 5);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 25);
    ASSERT_EQ_INT(board.move_count, 2);
    place_group_focus(&board, 4, GROUP_FIELD_SMALLER);
    press_digit(&board, TEST_OTHER_DIGIT);
    ASSERT_EQ_INT(board.move_count, 3);
    place_group_focus(&board, 4, GROUP_FIELD_GREATER);
    press_digit(&board, 0);
    ASSERT_EQ_INT(board.groups[4].greater, 0);
    ASSERT_EQ_INT(board.move_count, 4);
}

static void test_apply_board_input_operation_counts_a_move(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    place_group_focus(&board, 3, GROUP_FIELD_OPERATION);
    press_operation(&board, GROUP_OPERATION_ADD);
    ASSERT_EQ_INT(board.move_count, 1);
    press_operation(&board, GROUP_OPERATION_MULTIPLY);
    ASSERT_EQ_INT(board.groups[3].operation, GROUP_OPERATION_MULTIPLY);
    ASSERT_EQ_INT(board.move_count, 2);
    press_operation(&board, GROUP_OPERATION_MULTIPLY);
    ASSERT_EQ_INT(board.move_count, 2);
    press_operation(&board, GROUP_OPERATION_ADD);
    ASSERT_EQ_INT(board.move_count, 3);
}

static void test_apply_board_input_delete_counts_a_move(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, TEST_OTHER_DIGIT);
    ASSERT_EQ_INT(board.move_count, 1);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TETONOR_EMPTY_ENTRY);
    ASSERT_EQ_INT(board.move_count, 2);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.move_count, 2);

    place_group_focus(&board, 2, GROUP_FIELD_GREATER);
    press_digit(&board, 1);
    ASSERT_EQ_INT(board.move_count, 3);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.groups[2].greater, TETONOR_EMPTY_ENTRY);
    ASSERT_EQ_INT(board.move_count, 4);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.move_count, 4);

    place_group_focus(&board, 2, GROUP_FIELD_OPERATION);
    press_operation(&board, GROUP_OPERATION_ADD);
    ASSERT_EQ_INT(board.move_count, 5);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.groups[2].operation, GROUP_OPERATION_NONE);
    ASSERT_EQ_INT(board.move_count, 6);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.move_count, 6);
}

static void test_apply_board_input_unchanged_entries_are_not_moves(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 0);
    ASSERT_EQ_INT(board.move_count, 1);
    press_digit(&board, 0);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 0);
    ASSERT_EQ_INT(board.move_count, 1);

    press_digit(&board, TEST_OTHER_DIGIT);
    ASSERT_EQ_INT(board.move_count, 2);
    press_digit(&board, TEST_OVER_MAXIMUM_DIGIT);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TEST_OTHER_DIGIT);
    press_digit(&board, TEST_OUT_OF_RANGE_DIGIT);
    press_digit(&board, -1);
    ASSERT_EQ_INT(board.move_count, 2);

    place_group_focus(&board, 6, GROUP_FIELD_SMALLER);
    press_digit(&board, 2);
    ASSERT_EQ_INT(board.move_count, 3);
    press_digit(&board, TEST_OVER_MAXIMUM_DIGIT);
    ASSERT_EQ_INT(board.groups[6].smaller, 25);
    ASSERT_EQ_INT(board.move_count, 4);
    press_digit(&board, TEST_OVER_MAXIMUM_DIGIT);
    ASSERT_EQ_INT(board.groups[6].smaller, 25);
    ASSERT_EQ_INT(board.move_count, 4);

    place_group_focus(&board, 6, GROUP_FIELD_OPERATION);
    press_operation(&board, GROUP_OPERATION_NONE);
    ASSERT_EQ_INT(board.move_count, 4);
    press_digit(&board, 1);
    ASSERT_EQ_INT(board.move_count, 4);

    focus_slot(&board, TEST_SECOND_SLOT);
    press_operation(&board, GROUP_OPERATION_ADD);
    press(&board, BOARD_INPUT_ACTIVATE);
    place_group_focus(&board, 6, GROUP_FIELD_GREATER);
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.move_count, 4);
}

static void test_apply_board_input_movement_is_not_a_move(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, TEST_OTHER_DIGIT);
    ASSERT_EQ_INT(board.move_count, 1);
    for (int f = 0; f < TEST_FOCUS_COUNT; f++)
    {
        for (int k = 0; k < TEST_MOVE_KIND_COUNT; k++)
        {
            board.focus = fixture_focuses[f];
            press(&board, fixture_move_kinds[k]);
            ASSERT_EQ_INT(board.move_count, 1);
        }
    }
}

static void test_apply_board_input_confirm_does_not_count(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    board.focus = BOARD_FOCUS_SUBMIT;
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_EQ_INT(board.move_count, 0);

    start_wrong_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    ASSERT_EQ_INT(board.move_count, TEST_FILL_MOVE_COUNT);
    board.focus = BOARD_FOCUS_SUBMIT;
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    ASSERT_EQ_INT(board.move_count, TEST_FILL_MOVE_COUNT);

    start_correct_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    ASSERT_EQ_INT(board.move_count, TEST_FILL_MOVE_COUNT);
    board.focus = BOARD_FOCUS_SUBMIT;
    press(&board, BOARD_INPUT_ACTIVATE);
    ASSERT_EQ_INT(board.move_count, TEST_FILL_MOVE_COUNT);
    board.focus = BOARD_FOCUS_RESTART;
    ASSERT_EQ_INT(press_for_event(&board, BOARD_INPUT_ACTIVATE),
                  BOARD_EVENT_RESTART);
    ASSERT_EQ_INT(board.move_count, TEST_FILL_MOVE_COUNT);
}

static void test_apply_board_input_clearing_a_verdict_alone_is_not_a_move(void)
{
    NumberGeneration puzzle;
    Board board;
    start_wrong_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    empty_group_field(&board, 0, GROUP_FIELD_SMALLER);
    place_group_focus(&board, 0, GROUP_FIELD_SMALLER);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_EQ_INT(board.move_count, TEST_FILL_MOVE_COUNT);

    start_wrong_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    place_group_focus(&board, 0, GROUP_FIELD_OPERATION);
    press_operation(&board, GROUP_OPERATION_ADD);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_EQ_INT(board.move_count, TEST_FILL_MOVE_COUNT);

    start_wrong_board(&puzzle, &board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    focus_slot(&board, TEST_FIRST_SLOT);
    press_digit(&board, fixture_wrong_last_values[0]);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_EQ_INT(board.move_count, TEST_FILL_MOVE_COUNT);

    focus_slot(&board, TEST_FIRST_SLOT);
    press_digit(&board, TEST_OTHER_DIGIT);
    ASSERT_EQ_INT(board.move_count, TEST_FILL_MOVE_COUNT + 1);
}

static void test_apply_board_input_locked_board_counts_nothing(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    ASSERT_EQ_INT(board.move_count, TEST_FILL_MOVE_COUNT);
    const GroupOperation operations[] = {
        GROUP_OPERATION_ADD, GROUP_OPERATION_MULTIPLY, GROUP_OPERATION_NONE};
    for (int element = 0; element < TEST_LOCKED_FOCUS_COUNT; element++)
    {
        focus_element(&board, element);
        for (int digit = 0; digit < TETONOR_DIGIT_BASE; digit++)
        {
            press_digit(&board, digit);
        }
        press(&board, BOARD_INPUT_DELETE);
        for (size_t i = 0; i < sizeof operations / sizeof operations[0]; i++)
        {
            press_operation(&board, operations[i]);
        }
        press(&board, BOARD_INPUT_ACTIVATE);
        for (int k = 0; k < TEST_MOVE_KIND_COUNT; k++)
        {
            focus_element(&board, element);
            press(&board, fixture_move_kinds[k]);
        }
        ASSERT_EQ_INT(board.move_count, TEST_FILL_MOVE_COUNT);
    }
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void test_undo_board_move_keeps_move_count(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    for (int move = 0; move < 3; move++)
    {
        make_group_move(&board, move);
    }
    ASSERT_EQ_INT(board.move_count, 3);
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_EQ_INT(board.move_count, 3);
    ASSERT_EQ_INT(board.history.count, 1);
    make_group_move(&board, 3);
    ASSERT_EQ_INT(board.move_count, 4);
    ASSERT_EQ_INT(count_undos(&board), 2);
    ASSERT_FALSE(undo_board_move(&board));
    ASSERT_EQ_INT(board.move_count, 4);
}

static void test_undo_board_move_on_locked_board_keeps_move_count(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    ASSERT_FALSE(undo_board_move(&board));
    ASSERT_EQ_INT(board.move_count, TEST_FILL_MOVE_COUNT);
}

static void test_move_count_exceeds_undo_depth(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    for (int move = 0; move < TETONOR_UNDO_HISTORY_DEPTH + TEST_EXTRA_MOVES;
         move++)
    {
        make_group_move(&board, move);
    }
    ASSERT_EQ_INT(board.move_count,
                  TETONOR_UNDO_HISTORY_DEPTH + TEST_EXTRA_MOVES);
    ASSERT_EQ_INT(board.history.count, TETONOR_UNDO_HISTORY_DEPTH);

    init_board(&board, &puzzle);
    for (int move = 0; move < TEST_MANY_MOVES; move++)
    {
        make_slot_edit(&board, move);
    }
    ASSERT_EQ_INT(board.move_count, TEST_MANY_MOVES);
    ASSERT_EQ_INT(board.history.count, TETONOR_UNDO_HISTORY_DEPTH);
}

static void test_apply_board_input_move_count_saturates(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    board.move_count = INT_MAX - 1;
    make_group_move(&board, 0);
    ASSERT_EQ_INT(board.move_count, INT_MAX);
    make_group_move(&board, 1);
    ASSERT_EQ_INT(board.move_count, INT_MAX);
    make_group_move(&board, 2);
    ASSERT_EQ_INT(board.move_count, INT_MAX);
    ASSERT_EQ_INT(board.history.count, 3);
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_EQ_INT(board.move_count, INT_MAX);
}

static void test_board_move_count_returns_the_count(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    ASSERT_EQ_INT(board_move_count(&board), 0);
    make_group_move(&board, 0);
    make_group_move(&board, 1);
    ASSERT_EQ_INT(board_move_count(&board), 2);
    ASSERT_EQ_INT(board_move_count(&board), board.move_count);
    board.move_count = TEST_PRESET_MOVE_COUNT;
    ASSERT_EQ_INT(board_move_count(&board), TEST_PRESET_MOVE_COUNT);
    const Board* read_only = &board;
    ASSERT_EQ_INT(board_move_count(read_only), TEST_PRESET_MOVE_COUNT);
}

static void test_board_move_count_follows_a_solved_game(void)
{
    NumberGeneration puzzle;
    Board board;
    start_correct_board(&puzzle, &board);
    ASSERT_EQ_INT(board_move_count(&board), TEST_FILL_MOVE_COUNT);
    ASSERT_EQ_INT(board_move_count(&board), board.history.count);
}

int main(void)
{
    RUN_TEST(test_init_board_places_cursor_on_first_editable_slot);
    RUN_TEST(test_init_board_marks_hidden_numbers_as_editable);
    RUN_TEST(test_init_board_slots_start_empty_with_slot_focus);
    RUN_TEST(test_init_board_focus_starts_on_slots);
    RUN_TEST(test_init_board_result_is_pending);
    RUN_TEST(test_init_board_borrows_puzzle);
    RUN_TEST(test_init_board_groups_start_empty);
    RUN_TEST(test_init_board_group_focus_starts_on_first_smaller_box);
    RUN_TEST(test_init_board_discards_previous_session);
    RUN_TEST(test_init_board_without_editable_slots_has_no_cursor);
    RUN_TEST(test_apply_board_input_without_editable_slots_starts_on_groups);
    RUN_TEST(test_apply_board_input_without_editable_slots_never_focuses_slot);
    RUN_TEST(test_apply_board_input_without_editable_slots_down_reaches_submit);
    RUN_TEST(test_apply_board_input_without_editable_slots_submit_goes_back);
    RUN_TEST(test_apply_board_input_without_editable_slots_can_be_judged);
    RUN_TEST(
        test_apply_board_input_without_editable_slots_wrong_group_is_wrong);
    RUN_TEST(test_apply_board_input_move_right_visits_editable_slots_only);
    RUN_TEST(test_apply_board_input_move_right_past_last_slot_focuses_submit);
    RUN_TEST(test_apply_board_input_move_left_visits_editable_slots_only);
    RUN_TEST(test_apply_board_input_move_left_at_first_slot_stays);
    RUN_TEST(test_apply_board_input_move_with_single_editable_slot_stays);
    RUN_TEST(test_apply_board_input_move_keeps_typed_values);
    RUN_TEST(test_apply_board_input_move_down_focuses_submit_keeping_cursor);
    RUN_TEST(test_apply_board_input_move_up_on_slot_focuses_group);
    RUN_TEST(test_apply_board_input_move_up_restores_remembered_group_box);
    RUN_TEST(
        test_apply_board_input_move_down_from_last_group_row_returns_to_slot);
    RUN_TEST(test_apply_board_input_move_down_from_group_keeps_the_field);
    RUN_TEST(test_apply_board_input_move_up_from_group_keeps_the_field);
    RUN_TEST(test_apply_board_input_move_up_on_first_group_row_stays);
    RUN_TEST(test_apply_board_input_move_down_on_last_group_row_leaves_groups);
    RUN_TEST(test_apply_board_input_move_right_walks_the_boxes_of_a_row);
    RUN_TEST(test_apply_board_input_move_left_walks_the_boxes_of_a_row);
    RUN_TEST(test_apply_board_input_move_right_from_greater_enters_next_group);
    RUN_TEST(
        test_apply_board_input_move_left_from_smaller_enters_previous_group);
    RUN_TEST(test_apply_board_input_move_right_at_row_end_does_not_wrap);
    RUN_TEST(test_apply_board_input_move_left_at_row_start_does_not_wrap);
    RUN_TEST(test_apply_board_input_group_movement_keeps_every_value);
    RUN_TEST(test_apply_board_input_move_up_from_submit_returns_to_slot);
    RUN_TEST(test_apply_board_input_move_left_from_submit_returns_to_slot);
    RUN_TEST(test_apply_board_input_move_down_on_submit_focuses_restart);
    RUN_TEST(test_apply_board_input_move_right_on_submit_is_noop);
    RUN_TEST(test_apply_board_input_submit_up_keeps_remembered_group_box);
    RUN_TEST(test_apply_board_input_digit_on_empty_slot_sets_value);
    RUN_TEST(test_apply_board_input_digit_zero_is_a_value);
    RUN_TEST(test_apply_board_input_digit_only_changes_cursor_slot);
    RUN_TEST(test_apply_board_input_slot_digit_leaves_groups_empty);
    RUN_TEST(test_apply_board_input_two_digits_form_two_digit_number);
    RUN_TEST(test_apply_board_input_digits_reach_maximum_value);
    RUN_TEST(test_apply_board_input_digit_above_maximum_is_ignored);
    RUN_TEST(test_apply_board_input_digit_just_above_maximum_is_ignored);
    RUN_TEST(test_apply_board_input_digit_after_two_digits_is_ignored);
    RUN_TEST(test_apply_board_input_digit_after_ten_is_ignored);
    RUN_TEST(test_apply_board_input_leading_zero_is_replaced_by_next_digit);
    RUN_TEST(test_apply_board_input_repeated_zero_stays_zero);
    RUN_TEST(test_apply_board_input_digit_out_of_range_is_ignored);
    RUN_TEST(test_apply_board_input_digit_out_of_range_keeps_typed_value);
    RUN_TEST(test_apply_board_input_delete_removes_last_digit);
    RUN_TEST(test_apply_board_input_delete_on_empty_slot_is_noop);
    RUN_TEST(test_apply_board_input_delete_on_zero_empties_slot);
    RUN_TEST(test_apply_board_input_delete_on_ten_leaves_one);
    RUN_TEST(test_apply_board_input_typing_resumes_after_delete);
    RUN_TEST(test_apply_board_input_digit_on_submit_focus_is_noop);
    RUN_TEST(test_apply_board_input_delete_on_submit_focus_is_noop);
    RUN_TEST(test_apply_board_input_activate_on_slot_is_noop);
    RUN_TEST(test_apply_board_input_activate_on_slot_with_full_answer_is_noop);
    RUN_TEST(test_apply_board_input_activate_on_group_is_noop);
    RUN_TEST(test_apply_board_input_activate_on_group_keeps_result);
    RUN_TEST(test_apply_board_input_group_digit_sets_value);
    RUN_TEST(test_apply_board_input_group_digit_zero_is_a_value);
    RUN_TEST(test_apply_board_input_group_digit_does_not_touch_slots);
    RUN_TEST(test_apply_board_input_group_two_digits_form_number);
    RUN_TEST(test_apply_board_input_group_digits_reach_maximum_value);
    RUN_TEST(test_apply_board_input_group_digit_above_maximum_is_ignored);
    RUN_TEST(test_apply_board_input_group_digit_after_two_digits_is_ignored);
    RUN_TEST(test_apply_board_input_group_leading_zero_is_replaced);
    RUN_TEST(test_apply_board_input_group_repeated_zero_stays_zero);
    RUN_TEST(test_apply_board_input_group_digit_out_of_range_is_ignored);
    RUN_TEST(test_apply_board_input_group_digit_after_result_clears_result);
    RUN_TEST(test_apply_board_input_group_zero_after_result_replaces_value);
    RUN_TEST(test_apply_board_input_group_digit_out_of_range_keeps_result);
    RUN_TEST(test_apply_board_input_group_delete_removes_last_digit);
    RUN_TEST(test_apply_board_input_group_delete_on_empty_number_is_noop);
    RUN_TEST(test_apply_board_input_group_delete_on_zero_empties_box);
    RUN_TEST(test_apply_board_input_group_delete_on_ten_leaves_one);
    RUN_TEST(test_apply_board_input_group_typing_resumes_after_delete);
    RUN_TEST(test_apply_board_input_group_delete_after_result_clears_result);
    RUN_TEST(
        test_apply_board_input_group_delete_after_result_empties_single_digit);
    RUN_TEST(test_apply_board_input_operation_sets_the_operation_box);
    RUN_TEST(test_apply_board_input_operation_only_changes_the_focused_group);
    RUN_TEST(test_apply_board_input_operation_none_is_ignored);
    RUN_TEST(test_apply_board_input_operation_none_after_result_keeps_result);
    RUN_TEST(test_apply_board_input_operation_after_result_clears_result);
    RUN_TEST(test_apply_board_input_same_operation_after_result_clears_result);
    RUN_TEST(test_apply_board_input_delete_clears_the_operation_box);
    RUN_TEST(test_apply_board_input_delete_on_empty_operation_is_noop);
    RUN_TEST(test_apply_board_input_delete_on_operation_after_result_clears_it);
    RUN_TEST(test_apply_board_input_operation_on_group_number_is_ignored);
    RUN_TEST(test_apply_board_input_operation_on_empty_group_number_is_ignored);
    RUN_TEST(test_apply_board_input_operation_on_slot_is_ignored);
    RUN_TEST(
        test_apply_board_input_operation_on_slot_after_result_keeps_result);
    RUN_TEST(test_apply_board_input_operation_on_submit_is_ignored);
    RUN_TEST(test_apply_board_input_digit_on_operation_box_is_ignored);
    RUN_TEST(test_apply_board_input_digit_on_empty_operation_box_is_ignored);
    RUN_TEST(test_apply_board_input_group_by_inputs_enters_a_full_group);
    RUN_TEST(test_apply_board_input_whole_board_by_inputs_is_correct);
    RUN_TEST(test_apply_board_input_submit_accepts_zero_entry);
    RUN_TEST(test_apply_board_input_submit_judges_single_editable_slot);
    RUN_TEST(test_apply_board_input_submit_twice_keeps_result);
    RUN_TEST(test_apply_board_input_submit_twice_keeps_wrong_result);
    RUN_TEST(test_apply_board_input_submit_with_empty_slot_stays_pending);
    RUN_TEST(test_apply_board_input_submit_with_empty_first_slot_stays_pending);
    RUN_TEST(test_apply_board_input_submit_with_empty_groups_stays_pending);
    RUN_TEST(
        test_apply_board_input_submit_with_any_empty_group_box_stays_pending);
    RUN_TEST(test_apply_board_input_typing_never_judges);
    RUN_TEST(test_apply_board_input_all_filled_without_submit_stays_pending);
    RUN_TEST(test_apply_board_input_all_correct_is_correct);
    RUN_TEST(test_apply_board_input_one_wrong_entry_is_wrong);
    RUN_TEST(test_apply_board_input_swapped_entries_are_wrong);
    RUN_TEST(test_apply_board_input_filling_in_reverse_order_is_correct);
    RUN_TEST(test_apply_board_input_duplicate_of_given_value_is_accepted);
    RUN_TEST(test_apply_board_input_duplicate_entries_are_accepted);
    RUN_TEST(test_apply_board_input_wrong_operation_is_wrong);
    RUN_TEST(test_apply_board_input_smaller_above_greater_is_wrong);
    RUN_TEST(test_apply_board_input_wrong_group_number_is_wrong);
    RUN_TEST(
        test_apply_board_input_accepts_a_solution_other_than_the_generators);
    RUN_TEST(
        test_apply_board_input_digit_after_result_replaces_value_and_clears_result);
    RUN_TEST(test_apply_board_input_zero_after_result_replaces_value);
    RUN_TEST(test_apply_board_input_edit_after_result_keeps_other_values);
    RUN_TEST(
        test_apply_board_input_digit_out_of_range_after_result_keeps_result);
    RUN_TEST(
        test_apply_board_input_delete_after_result_removes_digit_and_clears_result);
    RUN_TEST(
        test_apply_board_input_delete_after_result_on_single_digit_empties);
    RUN_TEST(test_apply_board_input_resubmitting_fixed_entry_is_correct);
    RUN_TEST(test_apply_board_input_wrong_then_digit_fix_is_correct);
    RUN_TEST(test_apply_board_input_wrong_then_group_fix_is_correct);
    RUN_TEST(test_apply_board_input_editing_correct_result_is_locked);
    RUN_TEST(test_apply_board_input_editing_a_group_clears_a_wrong_result);
    RUN_TEST(test_apply_board_input_moving_cursor_keeps_result);
    RUN_TEST(test_apply_board_input_never_changes_given_numbers);
    RUN_TEST(test_init_board_slot_usages_start_unmarked);
    RUN_TEST(test_init_board_without_editable_slots_has_unmarked_usages);
    RUN_TEST(test_init_board_recomputes_usages_of_a_previous_session);
    RUN_TEST(test_init_board_fills_the_default_palette);
    RUN_TEST(test_init_board_restores_the_default_palette_of_a_used_board);
    RUN_TEST(test_apply_board_input_group_entry_marks_the_strip_live);
    RUN_TEST(test_apply_board_input_empty_hidden_slot_is_never_marked);
    RUN_TEST(test_apply_board_input_typing_a_used_value_marks_it_at_once);
    RUN_TEST(test_apply_board_input_typing_a_twice_used_value_marks_twice);
    RUN_TEST(test_apply_board_input_deleting_a_typed_value_unmarks_it);
    RUN_TEST(test_apply_board_input_extending_a_typed_value_updates_the_mark);
    RUN_TEST(test_apply_board_input_typed_unused_value_stays_unmarked);
    RUN_TEST(test_apply_board_input_add_and_multiply_group_mark_twice);
    RUN_TEST(test_apply_board_input_second_use_of_one_operation_stays_once);
    RUN_TEST(test_apply_board_input_third_use_keeps_the_box_twice);
    RUN_TEST(test_apply_board_input_two_adds_mark_equal_boxes_both_once);
    RUN_TEST(test_apply_board_input_equal_boxes_split_add_and_multiply);
    RUN_TEST(test_apply_board_input_equal_boxes_two_adds_and_a_multiply);
    RUN_TEST(test_apply_board_input_clearing_an_operation_lowers_the_marks);
    RUN_TEST(test_apply_board_input_clearing_an_operand_box_lowers_the_marks);
    RUN_TEST(test_apply_board_input_changing_an_operation_moves_the_mark);
    RUN_TEST(test_apply_board_input_changing_a_group_number_moves_the_mark);
    RUN_TEST(test_apply_board_input_wrong_result_leaves_marks_unchanged);
    RUN_TEST(test_apply_board_input_correct_result_leaves_marks_unchanged);
    RUN_TEST(test_apply_board_input_recomputes_marks_after_an_ignored_input);
    RUN_TEST(test_apply_board_input_recomputes_marks_after_every_input);
    RUN_TEST(test_apply_board_input_move_up_on_restart_focuses_submit);
    RUN_TEST(test_apply_board_input_activate_on_restart_requests_restart);
    RUN_TEST(
        test_apply_board_input_activate_on_restart_after_correct_requests_restart);
    RUN_TEST(
        test_apply_board_input_activate_on_restart_after_wrong_requests_restart);
    RUN_TEST(test_apply_board_input_activate_on_restart_does_not_judge);
    RUN_TEST(test_apply_board_input_noop_inputs_on_restart);
    RUN_TEST(test_apply_board_input_move_left_on_restart_returns_to_slot);
    RUN_TEST(
        test_apply_board_input_without_editable_slots_left_on_restart_focuses_group);
    RUN_TEST(test_apply_board_input_returns_none_outside_restart);
    RUN_TEST(test_apply_board_input_activate_on_submit_judges_and_returns_none);
    RUN_TEST(test_apply_board_input_slot_down_down_reaches_restart);
    RUN_TEST(test_apply_board_input_restart_up_up_returns_to_slot);
    RUN_TEST(test_undo_board_move_history_depth_is_twenty);
    RUN_TEST(test_undo_board_move_on_fresh_board_reports_nothing);
    RUN_TEST(test_undo_board_move_on_fresh_board_without_slots_reports_nothing);
    RUN_TEST(test_undo_board_move_on_empty_history_leaves_usages_untouched);
    RUN_TEST(test_undo_board_move_on_exhausted_history_reports_nothing);
    RUN_TEST(test_undo_board_move_reverts_typed_digit_in_slot);
    RUN_TEST(test_undo_board_move_reverts_typed_digit_in_group_box);
    RUN_TEST(test_undo_board_move_reverts_replaced_leading_zero_to_zero);
    RUN_TEST(test_undo_board_move_reverts_deleted_digit_in_slot);
    RUN_TEST(test_undo_board_move_reverts_deleted_digit_in_group_box);
    RUN_TEST(test_undo_board_move_reverts_operation);
    RUN_TEST(test_undo_board_move_recomputes_slot_usages);
    RUN_TEST(test_undo_board_move_restores_exact_entries_of_each_move);
    RUN_TEST(test_undo_board_move_keeps_cursor_and_focus);
    RUN_TEST(test_undo_board_move_works_from_every_focus);
    RUN_TEST(test_undo_board_move_reverts_moves_in_reverse_order);
    RUN_TEST(test_undo_board_move_keeps_only_the_last_twenty_moves);
    RUN_TEST(test_undo_board_move_keeps_exactly_twenty_moves_when_twenty_made);
    RUN_TEST(test_undo_board_move_wraps_after_many_moves);
    RUN_TEST(test_undo_board_move_survives_repeated_ring_wraparound);
    RUN_TEST(test_undo_board_move_then_new_move_is_undoable);
    RUN_TEST(test_apply_board_input_noop_inputs_add_no_history);
    RUN_TEST(test_apply_board_input_noop_inputs_do_not_evict_history);
    RUN_TEST(test_apply_board_input_setting_the_same_operation_is_not_a_move);
    RUN_TEST(test_apply_board_input_repeated_zero_is_not_a_move);
    RUN_TEST(test_apply_board_input_confirm_is_not_a_move);
    RUN_TEST(test_undo_board_move_restores_result_before_the_edit);
    RUN_TEST(test_apply_board_input_result_only_change_is_not_a_move);
    RUN_TEST(test_init_board_empties_move_history);
    RUN_TEST(test_init_board_empties_a_full_move_history);
    RUN_TEST(test_undo_board_move_without_editable_slots);
    RUN_TEST(test_undo_board_move_keeps_puzzle_palette_and_editable_flags);
    RUN_TEST(test_is_board_locked_follows_the_result);
    RUN_TEST(test_apply_board_input_locked_board_ignores_digits);
    RUN_TEST(test_apply_board_input_locked_board_ignores_operations);
    RUN_TEST(test_apply_board_input_locked_board_ignores_delete);
    RUN_TEST(test_apply_board_input_locked_board_ignores_confirm);
    RUN_TEST(test_apply_board_input_locked_board_activating_a_box_is_a_noop);
    RUN_TEST(test_apply_board_input_locked_board_allows_movement);
    RUN_TEST(test_apply_board_input_locked_board_movement_changes_focus);
    RUN_TEST(test_apply_board_input_locked_board_still_restarts);
    RUN_TEST(test_undo_board_move_locked_board_does_nothing);
    RUN_TEST(test_undo_board_move_works_again_after_a_new_game);
    RUN_TEST(test_board_progress_would_be_lost_rules);
    RUN_TEST(test_board_progress_would_be_lost_with_a_full_history);
    RUN_TEST(test_init_board_zeroes_move_count);
    RUN_TEST(test_apply_board_input_digit_counts_a_move);
    RUN_TEST(test_apply_board_input_operation_counts_a_move);
    RUN_TEST(test_apply_board_input_delete_counts_a_move);
    RUN_TEST(test_apply_board_input_unchanged_entries_are_not_moves);
    RUN_TEST(test_apply_board_input_movement_is_not_a_move);
    RUN_TEST(test_apply_board_input_confirm_does_not_count);
    RUN_TEST(test_apply_board_input_clearing_a_verdict_alone_is_not_a_move);
    RUN_TEST(test_apply_board_input_locked_board_counts_nothing);
    RUN_TEST(test_undo_board_move_keeps_move_count);
    RUN_TEST(test_undo_board_move_on_locked_board_keeps_move_count);
    RUN_TEST(test_move_count_exceeds_undo_depth);
    RUN_TEST(test_apply_board_input_move_count_saturates);
    RUN_TEST(test_board_move_count_returns_the_count);
    RUN_TEST(test_board_move_count_follows_a_solved_game);
    return test_harness_result();
}
