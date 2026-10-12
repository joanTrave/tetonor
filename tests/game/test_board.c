#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/game/board.h"

#define TEST_FIRST_SLOT 2
#define TEST_SECOND_SLOT 5
#define TEST_THIRD_SLOT 6
#define TEST_LAST_SLOT 15
#define TEST_EDITABLE_COUNT 4
#define TEST_OUT_OF_RANGE_DIGIT TETONOR_DIGIT_BASE
#define TEST_WRONG_VALUE 28

/* Eight pairs (1,2) (3,4) (5,6) (7,8) (9,10) (11,12) (13,14) (0,29), sorted;
 * index 15 holds the only two-digit value that is also the maximum. */
static const int fixture_solution[TETONOR_CELL_COUNT] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 29};
static const int fixture_sums[TETONOR_CELL_COUNT] = {
    3, 2, 7, 12, 11, 30, 15, 56, 19, 90, 23, 132, 27, 182, 29, 0};
static const int fixture_hidden[TEST_EDITABLE_COUNT] = {
    TEST_FIRST_SLOT, TEST_SECOND_SLOT, TEST_THIRD_SLOT, TEST_LAST_SLOT};
static const int fixture_hidden_values[TEST_EDITABLE_COUNT] = {2, 5, 6, 29};

static void build_puzzle(NumberGeneration* puzzle, const int* hidden,
                         size_t hidden_count)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        puzzle->sum_matrix[i / TETONOR_GRID_SIZE][i % TETONOR_GRID_SIZE] =
            fixture_sums[i];
        puzzle->available_array[i] = fixture_solution[i];
        puzzle->solution_array[i]  = fixture_solution[i];
    }
    for (size_t i = 0; i < hidden_count; i++)
    {
        puzzle->available_array[hidden[i]] = TETONOR_HIDDEN_NUMBER;
    }
}

static void start_default_board(NumberGeneration* puzzle, Board* board)
{
    build_puzzle(puzzle, fixture_hidden, TEST_EDITABLE_COUNT);
    init_board(board, puzzle);
}

static void press(Board* board, BoardInputKind kind)
{
    BoardInput input = {kind, 0};
    apply_board_input(board, input);
}

static void press_digit(Board* board, int digit)
{
    BoardInput input = {BOARD_INPUT_DIGIT, digit};
    apply_board_input(board, input);
}

static void type_and_confirm(Board* board, int value)
{
    if (value >= TETONOR_DIGIT_BASE)
    {
        press_digit(board, value / TETONOR_DIGIT_BASE);
    }
    press_digit(board, value % TETONOR_DIGIT_BASE);
    press(board, BOARD_INPUT_CONFIRM);
}

/* Starts on the first editable slot and leaves the cursor on the last. */
static void fill_all_slots(Board* board, const int values[TEST_EDITABLE_COUNT])
{
    for (int i = 0; i < TEST_EDITABLE_COUNT; i++)
    {
        type_and_confirm(board, values[i]);
        press(board, BOARD_INPUT_MOVE_RIGHT);
    }
}

static bool boards_equal(const Board* first, const Board* second)
{
    if (first->cursor != second->cursor || first->result != second->result)
    {
        return false;
    }
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        const BoardSlot* a = &first->slots[i];
        const BoardSlot* b = &second->slots[i];
        if (a->editable != b->editable || a->confirmed != b->confirmed ||
            (a->editable && a->value != b->value))
        {
            return false;
        }
    }
    return true;
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

static void test_init_board_slots_start_empty_and_unconfirmed(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        ASSERT_FALSE(board.slots[i].confirmed);
        if (board.slots[i].editable)
        {
            ASSERT_EQ_INT(board.slots[i].value, TETONOR_EMPTY_ENTRY);
        }
    }
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

static void test_init_board_without_editable_slots_has_no_cursor(void)
{
    NumberGeneration puzzle;
    Board board;
    build_puzzle(&puzzle, NULL, 0);
    init_board(&board, &puzzle);
    ASSERT_EQ_INT(board.cursor, TETONOR_NO_CURSOR);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        ASSERT_FALSE(board.slots[i].editable);
        ASSERT_FALSE(board.slots[i].confirmed);
    }
}

static void test_apply_board_input_without_editable_slots_is_noop(void)
{
    NumberGeneration puzzle;
    Board board;
    build_puzzle(&puzzle, NULL, 0);
    init_board(&board, &puzzle);
    Board before                 = board;
    const BoardInputKind kinds[] = {BOARD_INPUT_MOVE_LEFT,
                                    BOARD_INPUT_MOVE_RIGHT, BOARD_INPUT_DIGIT,
                                    BOARD_INPUT_DELETE, BOARD_INPUT_CONFIRM};
    for (size_t i = 0; i < sizeof kinds / sizeof kinds[0]; i++)
    {
        press_digit(&board, 5);
        press(&board, kinds[i]);
        ASSERT_TRUE(boards_equal(&board, &before));
    }
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

static void test_apply_board_input_move_right_at_last_slot_stays(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    for (int i = 0; i < TEST_EDITABLE_COUNT + 2; i++)
    {
        press(&board, BOARD_INPUT_MOVE_RIGHT);
    }
    ASSERT_EQ_INT(board.cursor, TEST_LAST_SLOT);
}

static void test_apply_board_input_move_left_visits_editable_slots_only(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    for (int i = 0; i < TEST_EDITABLE_COUNT; i++)
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
}

static void test_apply_board_input_move_with_single_editable_slot_stays(void)
{
    NumberGeneration puzzle;
    Board board;
    const int hidden[] = {TEST_THIRD_SLOT};
    build_puzzle(&puzzle, hidden, 1);
    init_board(&board, &puzzle);
    ASSERT_EQ_INT(board.cursor, TEST_THIRD_SLOT);
    press(&board, BOARD_INPUT_MOVE_LEFT);
    ASSERT_EQ_INT(board.cursor, TEST_THIRD_SLOT);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    ASSERT_EQ_INT(board.cursor, TEST_THIRD_SLOT);
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
    ASSERT_FALSE(board.slots[TEST_FIRST_SLOT].confirmed);
    ASSERT_EQ_INT(board.slots[TEST_SECOND_SLOT].value, TETONOR_EMPTY_ENTRY);
}

static void test_apply_board_input_digit_on_empty_slot_sets_value(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 7);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 7);
    ASSERT_FALSE(board.slots[TEST_FIRST_SLOT].confirmed);
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

static void test_apply_board_input_confirm_on_empty_slot_is_noop(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    Board before = board;
    press(&board, BOARD_INPUT_CONFIRM);
    ASSERT_FALSE(board.slots[TEST_FIRST_SLOT].confirmed);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void test_apply_board_input_confirm_marks_value_confirmed(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 2);
    press(&board, BOARD_INPUT_CONFIRM);
    ASSERT_TRUE(board.slots[TEST_FIRST_SLOT].confirmed);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 2);
    ASSERT_EQ_INT(board.cursor, TEST_FIRST_SLOT);
}

static void test_apply_board_input_confirm_accepts_zero(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 0);
    press(&board, BOARD_INPUT_CONFIRM);
    ASSERT_TRUE(board.slots[TEST_FIRST_SLOT].confirmed);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 0);
}

static void test_apply_board_input_confirm_twice_changes_nothing(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 2);
    press(&board, BOARD_INPUT_CONFIRM);
    Board before = board;
    press(&board, BOARD_INPUT_CONFIRM);
    ASSERT_TRUE(boards_equal(&board, &before));
}

static void test_apply_board_input_three_of_four_confirmed_stays_pending(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    for (int i = 0; i < TEST_EDITABLE_COUNT - 1; i++)
    {
        type_and_confirm(&board, fixture_hidden_values[i]);
        press(&board, BOARD_INPUT_MOVE_RIGHT);
    }
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void test_apply_board_input_wrong_entry_before_last_stays_pending(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    type_and_confirm(&board, TEST_WRONG_VALUE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void
test_apply_board_input_typed_but_unconfirmed_slot_keeps_pending(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    for (int i = 0; i < TEST_EDITABLE_COUNT - 1; i++)
    {
        type_and_confirm(&board, fixture_hidden_values[i]);
        press(&board, BOARD_INPUT_MOVE_RIGHT);
    }
    press_digit(&board, 2);
    press_digit(&board, 9);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void test_apply_board_input_all_correct_is_correct(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_all_slots(&board, fixture_hidden_values);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void test_apply_board_input_one_wrong_entry_is_wrong(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    const int values[TEST_EDITABLE_COUNT] = {2, 5, 6, TEST_WRONG_VALUE};
    fill_all_slots(&board, values);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
}

static void test_apply_board_input_swapped_entries_are_wrong(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    const int values[TEST_EDITABLE_COUNT] = {5, 2, 6, 29};
    fill_all_slots(&board, values);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
}

static void test_apply_board_input_confirming_in_reverse_order_is_correct(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    for (int i = 0; i < TEST_EDITABLE_COUNT; i++)
    {
        press(&board, BOARD_INPUT_MOVE_RIGHT);
    }
    for (int i = TEST_EDITABLE_COUNT - 1; i >= 0; i--)
    {
        type_and_confirm(&board, fixture_hidden_values[i]);
        press(&board, BOARD_INPUT_MOVE_LEFT);
    }
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void test_apply_board_input_duplicate_of_given_value_is_accepted(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    press_digit(&board, 1);
    press(&board, BOARD_INPUT_CONFIRM);
    ASSERT_TRUE(board.slots[TEST_FIRST_SLOT].confirmed);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, 1);
}

static void test_apply_board_input_duplicate_entries_are_accepted(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    const int values[TEST_EDITABLE_COUNT] = {5, 5, 5, 5};
    fill_all_slots(&board, values);
    ASSERT_TRUE(board.slots[TEST_FIRST_SLOT].confirmed);
    ASSERT_TRUE(board.slots[TEST_SECOND_SLOT].confirmed);
    ASSERT_EQ_INT(board.slots[TEST_SECOND_SLOT].value, 5);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
}

static void test_apply_board_input_digit_on_confirmed_slot_reopens_it(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    const int values[TEST_EDITABLE_COUNT] = {2, 5, 6, TEST_WRONG_VALUE};
    fill_all_slots(&board, values);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    press_digit(&board, 7);
    ASSERT_EQ_INT(board.slots[TEST_LAST_SLOT].value, 7);
    ASSERT_FALSE(board.slots[TEST_LAST_SLOT].confirmed);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void test_apply_board_input_reopen_keeps_other_slots_confirmed(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_all_slots(&board, fixture_hidden_values);
    press_digit(&board, 7);
    ASSERT_TRUE(board.slots[TEST_FIRST_SLOT].confirmed);
    ASSERT_TRUE(board.slots[TEST_SECOND_SLOT].confirmed);
    ASSERT_TRUE(board.slots[TEST_THIRD_SLOT].confirmed);
}

static void test_apply_board_input_delete_on_confirmed_slot_reopens_it(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    const int values[TEST_EDITABLE_COUNT] = {2, 5, 6, TEST_WRONG_VALUE};
    fill_all_slots(&board, values);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.slots[TEST_LAST_SLOT].value, 2);
    ASSERT_FALSE(board.slots[TEST_LAST_SLOT].confirmed);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
}

static void
test_apply_board_input_delete_on_confirmed_single_digit_empties(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    type_and_confirm(&board, 4);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.slots[TEST_FIRST_SLOT].value, TETONOR_EMPTY_ENTRY);
    ASSERT_FALSE(board.slots[TEST_FIRST_SLOT].confirmed);
}

static void test_apply_board_input_reconfirming_fixed_entry_is_correct(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    const int values[TEST_EDITABLE_COUNT] = {2, 5, 6, TEST_WRONG_VALUE};
    fill_all_slots(&board, values);
    press(&board, BOARD_INPUT_DELETE);
    press(&board, BOARD_INPUT_DELETE);
    type_and_confirm(&board, TETONOR_MAX_VALUE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void test_apply_board_input_reopening_correct_result_clears_it(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_all_slots(&board, fixture_hidden_values);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    press(&board, BOARD_INPUT_DELETE);
    type_and_confirm(&board, TEST_WRONG_VALUE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
}

static void test_apply_board_input_moving_cursor_keeps_result(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    fill_all_slots(&board, fixture_hidden_values);
    press(&board, BOARD_INPUT_MOVE_LEFT);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
}

static void test_apply_board_input_never_changes_given_numbers(void)
{
    NumberGeneration puzzle;
    Board board;
    start_default_board(&puzzle, &board);
    int given_before[TETONOR_CELL_COUNT];
    memcpy(given_before, puzzle.available_array, sizeof given_before);
    const int wrong_values[TEST_EDITABLE_COUNT] = {1, 0, 14, 7};
    fill_all_slots(&board, wrong_values);
    press_digit(&board, 9);
    press(&board, BOARD_INPUT_DELETE);
    for (int i = 0; i < TEST_EDITABLE_COUNT; i++)
    {
        press(&board, BOARD_INPUT_MOVE_LEFT);
    }
    fill_all_slots(&board, fixture_hidden_values);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_CORRECT);
    ASSERT_TRUE(
        memcmp(given_before, puzzle.available_array, sizeof given_before) == 0);
    ASSERT_TRUE(memcmp(fixture_solution, puzzle.solution_array,
                       sizeof fixture_solution) == 0);
}

int main(void)
{
    RUN_TEST(test_init_board_places_cursor_on_first_editable_slot);
    RUN_TEST(test_init_board_marks_hidden_numbers_as_editable);
    RUN_TEST(test_init_board_slots_start_empty_and_unconfirmed);
    RUN_TEST(test_init_board_result_is_pending);
    RUN_TEST(test_init_board_borrows_puzzle);
    RUN_TEST(test_init_board_without_editable_slots_has_no_cursor);
    RUN_TEST(test_apply_board_input_without_editable_slots_is_noop);
    RUN_TEST(test_apply_board_input_move_right_visits_editable_slots_only);
    RUN_TEST(test_apply_board_input_move_right_at_last_slot_stays);
    RUN_TEST(test_apply_board_input_move_left_visits_editable_slots_only);
    RUN_TEST(test_apply_board_input_move_left_at_first_slot_stays);
    RUN_TEST(test_apply_board_input_move_with_single_editable_slot_stays);
    RUN_TEST(test_apply_board_input_move_keeps_typed_values);
    RUN_TEST(test_apply_board_input_digit_on_empty_slot_sets_value);
    RUN_TEST(test_apply_board_input_digit_zero_is_a_value);
    RUN_TEST(test_apply_board_input_digit_only_changes_cursor_slot);
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
    RUN_TEST(test_apply_board_input_confirm_on_empty_slot_is_noop);
    RUN_TEST(test_apply_board_input_confirm_marks_value_confirmed);
    RUN_TEST(test_apply_board_input_confirm_accepts_zero);
    RUN_TEST(test_apply_board_input_confirm_twice_changes_nothing);
    RUN_TEST(test_apply_board_input_three_of_four_confirmed_stays_pending);
    RUN_TEST(test_apply_board_input_wrong_entry_before_last_stays_pending);
    RUN_TEST(test_apply_board_input_typed_but_unconfirmed_slot_keeps_pending);
    RUN_TEST(test_apply_board_input_all_correct_is_correct);
    RUN_TEST(test_apply_board_input_one_wrong_entry_is_wrong);
    RUN_TEST(test_apply_board_input_swapped_entries_are_wrong);
    RUN_TEST(test_apply_board_input_confirming_in_reverse_order_is_correct);
    RUN_TEST(test_apply_board_input_duplicate_of_given_value_is_accepted);
    RUN_TEST(test_apply_board_input_duplicate_entries_are_accepted);
    RUN_TEST(test_apply_board_input_digit_on_confirmed_slot_reopens_it);
    RUN_TEST(test_apply_board_input_reopen_keeps_other_slots_confirmed);
    RUN_TEST(test_apply_board_input_delete_on_confirmed_slot_reopens_it);
    RUN_TEST(test_apply_board_input_delete_on_confirmed_single_digit_empties);
    RUN_TEST(test_apply_board_input_reconfirming_fixed_entry_is_correct);
    RUN_TEST(test_apply_board_input_reopening_correct_result_clears_it);
    RUN_TEST(test_apply_board_input_moving_cursor_keeps_result);
    RUN_TEST(test_apply_board_input_never_changes_given_numbers);
    return test_harness_result();
}
