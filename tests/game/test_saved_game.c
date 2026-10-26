#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../test_harness.h"
#include "fixture_puzzle.h"
#include "fixture_saved_game.h"
#include "../../src/game/answer_check.h"
#include "../../src/game/board.h"
#include "../../src/game/game_timer.h"
#include "../../src/game/saved_game.h"

#define TEST_TEXT_SIZE (TETONOR_SAVED_GAME_TEXT_MAX + 1)
#define TEST_NOW ((time_t)5000)
#define TEST_OTHER_NOW ((time_t)900)
#define TEST_UNTOUCHED_BYTE 0x5a
#define TEST_ELAPSED 1234
#define TEST_SEED_COUNT 5
#define TEST_LEVEL_COUNT 3
#define TEST_EDIT_DIGIT 3
#define TEST_GROUP_SMALLER 1
#define TEST_GROUP_GREATER 2
#define TEST_NUMBER_FIELD_COUNT 2
#define TEST_BIG_VALUE 29
#define TEST_BIG_CELL 841
#define TEST_JUNK_LENGTH 4
#define TEST_NUL_POSITION 20

static const int test_levels[TEST_LEVEL_COUNT] = {
    DIFFICULTY_EASY, DIFFICULTY_MEDIUM, DIFFICULTY_HARD};

typedef struct
{
    const char* needle;
    const char* replacement;
} TextEdit;

static void remember_bytes(const void* object, size_t size, unsigned char* copy)
{
    memcpy(copy, object, size);
}

static bool bytes_unchanged(const void* object, size_t size,
                            const unsigned char* copy)
{
    return memcmp(object, copy, size) == 0;
}

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

/* Replaces the pair behind cells 2*pair and 2*pair+1 and rebuilds the matrix
 * and the strips around it, keeping which strip slots are hidden. */
static void set_pair(SavedGame* saved, int pair, int smaller, int greater)
{
    NumberGeneration* puzzle = &saved->puzzle;
    int sum_cell             = 2 * pair;
    int product_cell         = 2 * pair + 1;
    puzzle->solution_groups[sum_cell] =
        (OperandGroup){smaller, GROUP_OPERATION_ADD, greater};
    puzzle->solution_groups[product_cell] =
        (OperandGroup){smaller, GROUP_OPERATION_MULTIPLY, greater};
    puzzle->sum_matrix[sum_cell / TETONOR_GRID_SIZE]
                      [sum_cell % TETONOR_GRID_SIZE] = smaller + greater;
    puzzle->sum_matrix[product_cell / TETONOR_GRID_SIZE]
                      [product_cell % TETONOR_GRID_SIZE] = smaller * greater;
    int operands[TETONOR_CELL_COUNT];
    for (int i = 0; i < TETONOR_PAIR_COUNT; i++)
    {
        operands[2 * i]     = puzzle->solution_groups[2 * i].smaller;
        operands[2 * i + 1] = puzzle->solution_groups[2 * i].greater;
    }
    for (int i = 1; i < TETONOR_CELL_COUNT; i++)
    {
        for (int j = i; j > 0 && operands[j - 1] > operands[j]; j--)
        {
            int swapped     = operands[j];
            operands[j]     = operands[j - 1];
            operands[j - 1] = swapped;
        }
    }
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        bool hidden = puzzle->available_array[i] == TETONOR_HIDDEN_NUMBER;
        puzzle->solution_array[i] = operands[i];
        puzzle->available_array[i] =
            hidden ? TETONOR_HIDDEN_NUMBER : operands[i];
    }
}

static void expect_valid_after_pair(int smaller, int greater, bool valid)
{
    SavedGame saved;
    build_empty_saved_game(&saved, DIFFICULTY_EASY);
    ASSERT_TRUE(is_saved_game_valid(&saved));
    set_pair(&saved, 0, smaller, greater);
    ASSERT_EQ_INT(is_saved_game_valid(&saved), valid);
}

static void start_board(NumberGeneration* puzzle, Board* board, int difficulty)
{
    build_save_fixture_puzzle(puzzle, difficulty);
    init_board(board, puzzle);
}

/* White-box: the entries of the golden save, set directly on the board. */
static void type_golden_entries(Board* board)
{
    board->slots[SAVE_FIXTURE_TYPED_SLOT].value = SAVE_FIXTURE_TYPED_VALUE;
    board->slots[SAVE_FIXTURE_OTHER_TYPED_SLOT].value =
        SAVE_FIXTURE_OTHER_TYPED_VALUE;
    board->groups[0] = (OperandGroup){1, GROUP_OPERATION_ADD, 2};
    board->groups[1] = (OperandGroup){
        TETONOR_EMPTY_ENTRY, GROUP_OPERATION_MULTIPLY, TETONOR_EMPTY_ENTRY};
    board->groups[2] =
        (OperandGroup){12, GROUP_OPERATION_NONE, TETONOR_EMPTY_ENTRY};
    board->move_count = SAVE_FIXTURE_MOVES;
}

static bool boards_have_same_entries(const Board* first, const Board* second)
{
    if (first->result != second->result ||
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
    return true;
}

static void test_capture_saved_game_copies_the_game(void)
{
    NumberGeneration puzzle;
    Board board;
    start_board(&puzzle, &board, DIFFICULTY_EASY);
    type_golden_entries(&board);
    SavedGame expected;
    build_saved_game_fixture(&expected);
    SavedGame saved;
    memset(&saved, TEST_UNTOUCHED_BYTE, sizeof saved);
    ASSERT_TRUE(capture_saved_game(&board, SAVE_FIXTURE_ELAPSED,
                                   DIFFICULTY_EASY, &saved));
    ASSERT_TRUE(saved_games_equal(&saved, &expected));
}

static void test_capture_saved_game_keeps_the_given_difficulty_and_time(void)
{
    for (int i = 0; i < TEST_LEVEL_COUNT; i++)
    {
        NumberGeneration puzzle;
        Board board;
        start_board(&puzzle, &board, test_levels[i]);
        SavedGame saved;
        ASSERT_TRUE(
            capture_saved_game(&board, TEST_ELAPSED, test_levels[i], &saved));
        ASSERT_EQ_INT(saved.difficulty, test_levels[i]);
        ASSERT_EQ_INT(saved.elapsed_seconds, TEST_ELAPSED);
        ASSERT_EQ_INT(saved.move_count, 0);
        ASSERT_TRUE(number_generations_equal(&saved.puzzle, &puzzle));
    }
}

static void test_capture_saved_game_is_a_deep_copy(void)
{
    NumberGeneration puzzle;
    Board board;
    start_board(&puzzle, &board, DIFFICULTY_EASY);
    SavedGame saved;
    ASSERT_TRUE(capture_saved_game(&board, 0, DIFFICULTY_EASY, &saved));
    SavedGame before                           = saved;
    puzzle.available_array[0]                  = 99;
    puzzle.solution_array[0]                   = 99;
    puzzle.sum_matrix[0][0]                    = 99;
    puzzle.solution_groups[0].smaller          = 99;
    board.slots[SAVE_FIXTURE_TYPED_SLOT].value = TEST_BIG_VALUE;
    ASSERT_TRUE(saved_games_equal(&saved, &before));
}

static void test_capture_saved_game_takes_the_move_count_of_the_board(void)
{
    NumberGeneration puzzle;
    Board board;
    start_board(&puzzle, &board, DIFFICULTY_EASY);
    board.move_count = 40;
    SavedGame saved;
    ASSERT_TRUE(capture_saved_game(&board, 0, DIFFICULTY_EASY, &saved));
    ASSERT_EQ_INT(saved.move_count, 40);
}

static void test_capture_saved_game_accepts_a_wrong_answer(void)
{
    SavedGame source;
    build_empty_saved_game(&source, DIFFICULTY_EASY);
    fill_saved_game_with_wrong_answer(&source);
    NumberGeneration puzzle = source.puzzle;
    Board board;
    init_board(&board, &puzzle);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        board.slots[i].value = source.entries.slot_values[i];
        board.groups[i]      = source.entries.groups[i];
    }
    board.result     = BOARD_RESULT_WRONG;
    board.move_count = source.move_count;
    SavedGame saved;
    ASSERT_TRUE(capture_saved_game(&board, 0, DIFFICULTY_EASY, &saved));
    ASSERT_EQ_INT(saved.entries.result, BOARD_RESULT_WRONG);
    ASSERT_TRUE(snapshots_equal(&saved.entries, &source.entries));
}

static void test_capture_saved_game_locked_board_fails_untouched(void)
{
    NumberGeneration puzzle;
    Board board;
    start_board(&puzzle, &board, DIFFICULTY_EASY);
    board.result = BOARD_RESULT_CORRECT;
    SavedGame saved;
    memset(&saved, TEST_UNTOUCHED_BYTE, sizeof saved);
    unsigned char before[sizeof saved];
    remember_bytes(&saved, sizeof saved, before);
    ASSERT_FALSE(capture_saved_game(&board, 0, DIFFICULTY_EASY, &saved));
    ASSERT_TRUE(bytes_unchanged(&saved, sizeof saved, before));
}

static void test_capture_saved_game_without_a_puzzle_fails_untouched(void)
{
    NumberGeneration puzzle;
    Board board;
    start_board(&puzzle, &board, DIFFICULTY_EASY);
    board.puzzle = NULL;
    SavedGame saved;
    memset(&saved, TEST_UNTOUCHED_BYTE, sizeof saved);
    unsigned char before[sizeof saved];
    remember_bytes(&saved, sizeof saved, before);
    ASSERT_FALSE(capture_saved_game(&board, 0, DIFFICULTY_EASY, &saved));
    ASSERT_TRUE(bytes_unchanged(&saved, sizeof saved, before));
}

static void test_capture_saved_game_invalid_game_fails_untouched(void)
{
    NumberGeneration puzzle;
    Board board;
    start_board(&puzzle, &board, DIFFICULTY_EASY);
    SavedGame saved;
    memset(&saved, TEST_UNTOUCHED_BYTE, sizeof saved);
    unsigned char before[sizeof saved];
    remember_bytes(&saved, sizeof saved, before);
    ASSERT_FALSE(capture_saved_game(&board, -1, DIFFICULTY_EASY, &saved));
    ASSERT_FALSE(capture_saved_game(&board, TETONOR_TIMER_MAX_SECONDS + 1,
                                    DIFFICULTY_EASY, &saved));
    ASSERT_FALSE(capture_saved_game(&board, 0, DIFFICULTY_COUNT, &saved));
    ASSERT_FALSE(capture_saved_game(&board, 0, -1, &saved));
    ASSERT_FALSE(capture_saved_game(&board, 0, DIFFICULTY_MEDIUM, &saved));
    ASSERT_TRUE(bytes_unchanged(&saved, sizeof saved, before));
    board.slots[SAVE_FIXTURE_TYPED_SLOT].value = TETONOR_MAX_VALUE + 1;
    board.move_count                           = 1;
    ASSERT_FALSE(capture_saved_game(&board, 0, DIFFICULTY_EASY, &saved));
    ASSERT_TRUE(bytes_unchanged(&saved, sizeof saved, before));
}

static void test_is_saved_game_valid_accepts_the_fixture(void)
{
    SavedGame saved;
    build_saved_game_fixture(&saved);
    ASSERT_TRUE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_accepts_every_difficulty(void)
{
    for (int i = 0; i < TEST_LEVEL_COUNT; i++)
    {
        SavedGame saved;
        build_empty_saved_game(&saved, test_levels[i]);
        ASSERT_TRUE(is_saved_game_valid(&saved));
        fill_saved_game_with_solution(&saved);
        ASSERT_TRUE(is_saved_game_valid(&saved));
        fill_saved_game_with_wrong_answer(&saved);
        ASSERT_TRUE(is_saved_game_valid(&saved));
    }
}

static void test_is_saved_game_valid_difficulty_must_be_a_level(void)
{
    SavedGame saved;
    build_empty_saved_game(&saved, DIFFICULTY_EASY);
    saved.difficulty = -1;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    saved.difficulty = INT_MIN;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    build_empty_saved_game(&saved, DIFFICULTY_HARD);
    saved.difficulty = DIFFICULTY_COUNT;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    saved.difficulty = INT_MAX;
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void
test_is_saved_game_valid_difficulty_must_match_the_hidden_count(void)
{
    SavedGame saved;
    build_empty_saved_game(&saved, DIFFICULTY_EASY);
    saved.difficulty = DIFFICULTY_MEDIUM;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    saved.difficulty = DIFFICULTY_HARD;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    build_empty_saved_game(&saved, DIFFICULTY_HARD);
    saved.difficulty = DIFFICULTY_EASY;
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_elapsed_bounds(void)
{
    SavedGame saved;
    build_saved_game_fixture(&saved);
    saved.elapsed_seconds = 0;
    ASSERT_TRUE(is_saved_game_valid(&saved));
    saved.elapsed_seconds = TETONOR_TIMER_MAX_SECONDS;
    ASSERT_TRUE(is_saved_game_valid(&saved));
    saved.elapsed_seconds = -1;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    saved.elapsed_seconds = TETONOR_TIMER_MAX_SECONDS + 1;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    saved.elapsed_seconds = INT32_MAX;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    saved.elapsed_seconds = INT32_MIN;
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_move_count_bounds(void)
{
    SavedGame saved;
    build_saved_game_fixture(&saved);
    saved.move_count = INT_MAX;
    ASSERT_TRUE(is_saved_game_valid(&saved));
    build_empty_saved_game(&saved, DIFFICULTY_EASY);
    saved.move_count = 0;
    ASSERT_TRUE(is_saved_game_valid(&saved));
    saved.move_count = -1;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    saved.move_count = INT_MIN;
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_solution_range_per_level(void)
{
    expect_valid_after_pair(1, 2, true);
    expect_valid_after_pair(0, 2, false);
    expect_valid_after_pair(1, 12, true);
    expect_valid_after_pair(1, 13, false);
    SavedGame saved;
    build_empty_saved_game(&saved, DIFFICULTY_MEDIUM);
    set_pair(&saved, 0, 1, 20);
    ASSERT_TRUE(is_saved_game_valid(&saved));
    set_pair(&saved, 0, 1, 21);
    ASSERT_FALSE(is_saved_game_valid(&saved));
    build_empty_saved_game(&saved, DIFFICULTY_HARD);
    set_pair(&saved, 0, 1, TETONOR_MAX_VALUE);
    ASSERT_TRUE(is_saved_game_valid(&saved));
    set_pair(&saved, 0, 1, TETONOR_MAX_VALUE + 1);
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_solution_must_be_sorted(void)
{
    SavedGame saved;
    build_empty_saved_game(&saved, DIFFICULTY_EASY);
    int swapped                      = saved.puzzle.solution_array[14];
    saved.puzzle.solution_array[14]  = saved.puzzle.solution_array[15];
    saved.puzzle.solution_array[15]  = swapped;
    saved.puzzle.available_array[14] = saved.puzzle.solution_array[14];
    saved.puzzle.available_array[15] = saved.puzzle.solution_array[15];
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_hidden_count_must_match_the_level(void)
{
    SavedGame saved;
    build_empty_saved_game(&saved, DIFFICULTY_EASY);
    saved.puzzle.available_array[0] = TETONOR_HIDDEN_NUMBER;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    build_empty_saved_game(&saved, DIFFICULTY_EASY);
    saved.puzzle.available_array[1] = saved.puzzle.solution_array[1];
    ASSERT_FALSE(is_saved_game_valid(&saved));
    build_empty_saved_game(&saved, DIFFICULTY_HARD);
    saved.puzzle.available_array[0] = saved.puzzle.solution_array[0];
    ASSERT_FALSE(is_saved_game_valid(&saved));
    build_empty_saved_game(&saved, DIFFICULTY_MEDIUM);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        saved.puzzle.available_array[i] = TETONOR_HIDDEN_NUMBER;
    }
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_visible_value_must_be_the_solution(void)
{
    SavedGame saved;
    build_empty_saved_game(&saved, DIFFICULTY_EASY);
    saved.puzzle.available_array[0] = saved.puzzle.solution_array[0] + 1;
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_solution_must_solve_the_matrix(void)
{
    SavedGame saved;
    build_saved_game_fixture(&saved);
    saved.puzzle.solution_groups[0].greater++;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    build_saved_game_fixture(&saved);
    saved.puzzle.solution_groups[1].operation = GROUP_OPERATION_ADD;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    build_saved_game_fixture(&saved);
    saved.puzzle.sum_matrix[0][0]++;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    build_saved_game_fixture(&saved);
    saved.puzzle.sum_matrix[TETONOR_GRID_SIZE - 1][TETONOR_GRID_SIZE - 1] =
        TEST_BIG_CELL;
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_entry_only_in_a_hidden_slot(void)
{
    SavedGame saved;
    build_saved_game_fixture(&saved);
    saved.entries.slot_values[0] = 1;
    saved.move_count++;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    build_saved_game_fixture(&saved);
    saved.entries.slot_values[2] = 1;
    saved.move_count++;
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_slot_value_bounds(void)
{
    SavedGame saved;
    build_saved_game_fixture(&saved);
    saved.entries.slot_values[SAVE_FIXTURE_TYPED_SLOT] = TETONOR_MIN_VALUE;
    ASSERT_TRUE(is_saved_game_valid(&saved));
    saved.entries.slot_values[SAVE_FIXTURE_TYPED_SLOT] = TETONOR_MAX_VALUE;
    ASSERT_TRUE(is_saved_game_valid(&saved));
    saved.entries.slot_values[SAVE_FIXTURE_TYPED_SLOT] = TETONOR_MAX_VALUE + 1;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    saved.entries.slot_values[SAVE_FIXTURE_TYPED_SLOT] =
        TETONOR_EMPTY_ENTRY - 1;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    saved.entries.slot_values[SAVE_FIXTURE_TYPED_SLOT] = INT_MAX;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    saved.entries.slot_values[SAVE_FIXTURE_TYPED_SLOT] = INT_MIN;
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_group_number_bounds(void)
{
    for (int field = 0; field < TEST_NUMBER_FIELD_COUNT; field++)
    {
        SavedGame saved;
        build_saved_game_fixture(&saved);
        int* box = field == 0 ? &saved.entries.groups[3].smaller
                              : &saved.entries.groups[3].greater;
        saved.move_count++;
        *box = TETONOR_MIN_VALUE;
        ASSERT_TRUE(is_saved_game_valid(&saved));
        *box = TETONOR_MAX_VALUE;
        ASSERT_TRUE(is_saved_game_valid(&saved));
        *box = TETONOR_MAX_VALUE + 1;
        ASSERT_FALSE(is_saved_game_valid(&saved));
        *box = TETONOR_EMPTY_ENTRY - 1;
        ASSERT_FALSE(is_saved_game_valid(&saved));
        *box = INT_MAX;
        ASSERT_FALSE(is_saved_game_valid(&saved));
        *box = INT_MIN;
        ASSERT_FALSE(is_saved_game_valid(&saved));
    }
}

static void test_is_saved_game_valid_group_operation_must_be_known(void)
{
    SavedGame saved;
    build_saved_game_fixture(&saved);
    saved.move_count++;
    saved.entries.groups[3].operation = GROUP_OPERATION_ADD;
    ASSERT_TRUE(is_saved_game_valid(&saved));
    saved.entries.groups[3].operation = GROUP_OPERATION_MULTIPLY;
    ASSERT_TRUE(is_saved_game_valid(&saved));
    saved.entries.groups[3].operation = (GroupOperation)-1;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    saved.entries.groups[3].operation = (GroupOperation)3;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    saved.entries.groups[3].operation = (GroupOperation)INT_MAX;
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_result_pending_or_wrong_only(void)
{
    SavedGame saved;
    build_empty_saved_game(&saved, DIFFICULTY_EASY);
    fill_saved_game_with_solution(&saved);
    ASSERT_TRUE(is_saved_game_valid(&saved));
    saved.entries.result = BOARD_RESULT_CORRECT;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    saved.entries.result = (BoardResult)-1;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    saved.entries.result = (BoardResult)INT_MAX;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    build_saved_game_fixture(&saved);
    saved.entries.result = BOARD_RESULT_CORRECT;
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_wrong_result_needs_a_wrong_answer(void)
{
    SavedGame saved;
    build_empty_saved_game(&saved, DIFFICULTY_EASY);
    fill_saved_game_with_wrong_answer(&saved);
    ASSERT_TRUE(is_saved_game_valid(&saved));
    saved.entries.groups[0].operation = GROUP_OPERATION_ADD;
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_wrong_result_needs_every_box_filled(void)
{
    SavedGame saved;
    build_empty_saved_game(&saved, DIFFICULTY_EASY);
    fill_saved_game_with_wrong_answer(&saved);
    saved.entries.slot_values[SAVE_FIXTURE_TYPED_SLOT] = TETONOR_EMPTY_ENTRY;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    fill_saved_game_with_wrong_answer(&saved);
    saved.entries.groups[7].smaller = TETONOR_EMPTY_ENTRY;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    fill_saved_game_with_wrong_answer(&saved);
    saved.entries.groups[8].operation = GROUP_OPERATION_NONE;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    fill_saved_game_with_wrong_answer(&saved);
    saved.entries.groups[15].greater = TETONOR_EMPTY_ENTRY;
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_pending_may_hold_a_correct_answer(void)
{
    SavedGame saved;
    build_empty_saved_game(&saved, DIFFICULTY_EASY);
    fill_saved_game_with_solution(&saved);
    ASSERT_EQ_INT(saved.entries.result, BOARD_RESULT_PENDING);
    ASSERT_TRUE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_wrong_result_with_a_correct_answer(void)
{
    SavedGame saved;
    build_empty_saved_game(&saved, DIFFICULTY_EASY);
    fill_saved_game_with_solution(&saved);
    saved.entries.result = BOARD_RESULT_WRONG;
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_wrong_result_judges_typed_slots(void)
{
    SavedGame saved;
    build_empty_saved_game(&saved, DIFFICULTY_EASY);
    fill_saved_game_with_solution(&saved);
    saved.entries.result = BOARD_RESULT_WRONG;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    saved.entries.slot_values[SAVE_FIXTURE_TYPED_SLOT] =
        saved.puzzle.solution_array[SAVE_FIXTURE_TYPED_SLOT] + 1;
    ASSERT_TRUE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_moves_cover_the_filled_boxes(void)
{
    SavedGame saved;
    build_saved_game_fixture(&saved);
    ASSERT_EQ_INT(count_saved_game_filled_boxes(&saved), SAVE_FIXTURE_MOVES);
    saved.move_count = SAVE_FIXTURE_MOVES;
    ASSERT_TRUE(is_saved_game_valid(&saved));
    saved.move_count = SAVE_FIXTURE_MOVES + 1;
    ASSERT_TRUE(is_saved_game_valid(&saved));
    saved.move_count = SAVE_FIXTURE_MOVES - 1;
    ASSERT_FALSE(is_saved_game_valid(&saved));
    saved.move_count = 0;
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void test_is_saved_game_valid_moves_cover_a_full_board(void)
{
    SavedGame saved;
    build_empty_saved_game(&saved, DIFFICULTY_HARD);
    fill_saved_game_with_solution(&saved);
    ASSERT_TRUE(is_saved_game_valid(&saved));
    saved.move_count--;
    ASSERT_FALSE(is_saved_game_valid(&saved));
}

static void test_serialize_saved_game_writes_the_documented_text(void)
{
    SavedGame saved;
    build_saved_game_fixture(&saved);
    char text[TEST_TEXT_SIZE];
    ASSERT_TRUE(serialize_saved_game(&saved, text, sizeof text));
    ASSERT_TRUE(strcmp(text, SAVE_FIXTURE_TEXT) == 0);
}

static void test_serialize_saved_game_writes_the_wrong_result_word(void)
{
    SavedGame saved;
    build_empty_saved_game(&saved, DIFFICULTY_EASY);
    fill_saved_game_with_wrong_answer(&saved);
    char text[TEST_TEXT_SIZE];
    ASSERT_TRUE(serialize_saved_game(&saved, text, sizeof text));
    ASSERT_NOT_NULL(strstr(text, "\nresult wrong\n"));
    ASSERT_NULL(strstr(text, "pending"));
}

static void test_serialize_saved_game_every_smaller_size_fails(void)
{
    SavedGame saved;
    build_saved_game_fixture(&saved);
    size_t needed = strlen(SAVE_FIXTURE_TEXT) + 1;
    for (size_t size = 0; size < needed; size++)
    {
        char buffer[TEST_TEXT_SIZE + 1];
        memset(buffer, TEST_UNTOUCHED_BYTE, sizeof buffer);
        ASSERT_FALSE(serialize_saved_game(&saved, buffer, size));
        for (size_t at = size; at < sizeof buffer; at++)
        {
            ASSERT_EQ_INT((unsigned char)buffer[at], TEST_UNTOUCHED_BYTE);
        }
    }
}

static void test_serialize_saved_game_exact_size_fits(void)
{
    SavedGame saved;
    build_saved_game_fixture(&saved);
    size_t needed = strlen(SAVE_FIXTURE_TEXT) + 1;
    char buffer[TEST_TEXT_SIZE + 1];
    memset(buffer, TEST_UNTOUCHED_BYTE, sizeof buffer);
    ASSERT_TRUE(serialize_saved_game(&saved, buffer, needed));
    ASSERT_EQ_INT(buffer[needed - 1], '\0');
    ASSERT_EQ_INT((unsigned char)buffer[needed], TEST_UNTOUCHED_BYTE);
    ASSERT_TRUE(strcmp(buffer, SAVE_FIXTURE_TEXT) == 0);
}

static void test_serialize_saved_game_widest_game_fits_the_file_limit(void)
{
    SavedGame saved;
    memset(&saved, 0, sizeof saved);
    saved.difficulty      = DIFFICULTY_HARD;
    saved.elapsed_seconds = TETONOR_TIMER_MAX_SECONDS;
    saved.move_count      = INT_MAX;
    saved.entries.result  = BOARD_RESULT_WRONG;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        saved.puzzle.sum_matrix[i / TETONOR_GRID_SIZE][i % TETONOR_GRID_SIZE] =
            TEST_BIG_CELL;
        saved.puzzle.available_array[i] = TETONOR_MAX_VALUE;
        saved.puzzle.solution_array[i]  = TETONOR_MAX_VALUE;
        saved.puzzle.solution_groups[i] = (OperandGroup){
            TETONOR_MAX_VALUE, GROUP_OPERATION_MULTIPLY, TETONOR_MAX_VALUE};
        saved.entries.slot_values[i] = TETONOR_MAX_VALUE;
        saved.entries.groups[i]      = saved.puzzle.solution_groups[i];
    }
    char text[TEST_TEXT_SIZE];
    ASSERT_TRUE(serialize_saved_game(&saved, text, sizeof text));
    ASSERT_TRUE(strlen(text) <= TETONOR_SAVED_GAME_TEXT_MAX);
}

static void expect_text_parses_to_the_fixture(const char* text, size_t length)
{
    SavedGame expected;
    build_saved_game_fixture(&expected);
    SavedGame parsed;
    memset(&parsed, TEST_UNTOUCHED_BYTE, sizeof parsed);
    ASSERT_TRUE(parse_saved_game(text, length, &parsed));
    ASSERT_TRUE(saved_games_equal(&parsed, &expected));
}

static void expect_text_rejected(const char* text, size_t length)
{
    SavedGame saved;
    memset(&saved, TEST_UNTOUCHED_BYTE, sizeof saved);
    unsigned char before[sizeof saved];
    remember_bytes(&saved, sizeof saved, before);
    ASSERT_FALSE(parse_saved_game(text, length, &saved));
    ASSERT_TRUE(bytes_unchanged(&saved, sizeof saved, before));
}

/* The golden text with the first occurrence of needle replaced. */
static void apply_edit(const TextEdit* edit, char* out, size_t size,
                       size_t* length)
{
    const char* golden = SAVE_FIXTURE_TEXT;
    const char* at     = strstr(golden, edit->needle);
    ASSERT_NOT_NULL(at);
    size_t head = (size_t)(at - golden);
    size_t tail = strlen(at + strlen(edit->needle));
    ASSERT_TRUE(head + strlen(edit->replacement) + tail < size);
    memcpy(out, golden, head);
    memcpy(out + head, edit->replacement, strlen(edit->replacement));
    memcpy(out + head + strlen(edit->replacement), at + strlen(edit->needle),
           tail + 1);
    *length = head + strlen(edit->replacement) + tail;
}

static void test_parse_saved_game_round_trips_the_fixture(void)
{
    expect_text_parses_to_the_fixture(SAVE_FIXTURE_TEXT,
                                      strlen(SAVE_FIXTURE_TEXT));
}

static void test_parse_saved_game_round_trips_every_difficulty(void)
{
    for (int i = 0; i < TEST_LEVEL_COUNT; i++)
    {
        SavedGame source;
        build_empty_saved_game(&source, test_levels[i]);
        source.elapsed_seconds = TEST_ELAPSED;
        char text[TEST_TEXT_SIZE];
        ASSERT_TRUE(serialize_saved_game(&source, text, sizeof text));
        SavedGame parsed;
        ASSERT_TRUE(parse_saved_game(text, strlen(text), &parsed));
        ASSERT_TRUE(saved_games_equal(&parsed, &source));
        fill_saved_game_with_solution(&source);
        ASSERT_TRUE(serialize_saved_game(&source, text, sizeof text));
        ASSERT_TRUE(parse_saved_game(text, strlen(text), &parsed));
        ASSERT_TRUE(saved_games_equal(&parsed, &source));
        fill_saved_game_with_wrong_answer(&source);
        ASSERT_TRUE(serialize_saved_game(&source, text, sizeof text));
        ASSERT_TRUE(parse_saved_game(text, strlen(text), &parsed));
        ASSERT_TRUE(saved_games_equal(&parsed, &source));
    }
}

static void test_parse_saved_game_accepts_crlf_line_ends(void)
{
    char text[TEST_TEXT_SIZE];
    size_t length = 0;
    for (const char* at = SAVE_FIXTURE_TEXT; *at != '\0'; at++)
    {
        if (*at == '\n')
        {
            text[length++] = '\r';
        }
        text[length++] = *at;
    }
    text[length] = '\0';
    expect_text_parses_to_the_fixture(text, length);
}

static void test_parse_saved_game_accepts_a_last_line_without_line_end(void)
{
    size_t length = strlen(SAVE_FIXTURE_TEXT);
    expect_text_parses_to_the_fixture(SAVE_FIXTURE_TEXT, length - 1);
}

static void test_parse_saved_game_reads_only_the_given_length(void)
{
    char text[TEST_TEXT_SIZE];
    size_t length = strlen(SAVE_FIXTURE_TEXT);
    memcpy(text, SAVE_FIXTURE_TEXT, length);
    memcpy(text + length, "junk", TEST_JUNK_LENGTH);
    expect_text_parses_to_the_fixture(text, length);
    expect_text_rejected(text, length + TEST_JUNK_LENGTH);
}

static void test_parse_saved_game_accepts_the_value_boundaries(void)
{
    const struct
    {
        TextEdit edit;
        int slot_value;
        int elapsed;
        int moves;
    } cases[] = {
        {{"elapsed 754", "elapsed 0"}, 1, 0, SAVE_FIXTURE_MOVES},
        {{"elapsed 754", "elapsed 359999"}, 1, 359999, SAVE_FIXTURE_MOVES},
        {{"moves 7", "moves 2147483647"}, 1, SAVE_FIXTURE_ELAPSED, INT_MAX},
        {{"slots - 1", "slots - 0"},
         0,
         SAVE_FIXTURE_ELAPSED,
         SAVE_FIXTURE_MOVES},
        {{"slots - 1", "slots - 29"},
         29,
         SAVE_FIXTURE_ELAPSED,
         SAVE_FIXTURE_MOVES},
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++)
    {
        char text[TEST_TEXT_SIZE];
        size_t length = 0;
        apply_edit(&cases[i].edit, text, sizeof text, &length);
        SavedGame parsed;
        ASSERT_TRUE(parse_saved_game(text, length, &parsed));
        ASSERT_EQ_INT(parsed.elapsed_seconds, cases[i].elapsed);
        ASSERT_EQ_INT(parsed.move_count, cases[i].moves);
        ASSERT_EQ_INT(parsed.entries.slot_values[SAVE_FIXTURE_TYPED_SLOT],
                      cases[i].slot_value);
    }
}

static void test_parse_saved_game_rejects_empty_and_header_only_text(void)
{
    expect_text_rejected("", 0);
    expect_text_rejected("TETONOR-SAVE 1", strlen("TETONOR-SAVE 1"));
    expect_text_rejected("TETONOR-SAVE 1\n", strlen("TETONOR-SAVE 1\n"));
    expect_text_rejected("\n", 1);
}

static void test_parse_saved_game_rejects_every_truncation(void)
{
    size_t full = strlen(SAVE_FIXTURE_TEXT);
    for (size_t length = 0; length + 1 < full; length++)
    {
        char* exact = malloc(length == 0 ? 1 : length);
        ASSERT_NOT_NULL(exact);
        memcpy(exact, SAVE_FIXTURE_TEXT, length);
        SavedGame saved;
        memset(&saved, TEST_UNTOUCHED_BYTE, sizeof saved);
        unsigned char before[sizeof saved];
        remember_bytes(&saved, sizeof saved, before);
        bool accepted  = parse_saved_game(exact, length, &saved);
        bool untouched = bytes_unchanged(&saved, sizeof saved, before);
        free(exact);
        ASSERT_FALSE(accepted);
        ASSERT_TRUE(untouched);
    }
}

static void test_parse_saved_game_rejects_trailing_bytes(void)
{
    const char* suffixes[] = {"x", "\n", " ", "\r\n", "TETONOR-SAVE 1\n"};
    for (size_t i = 0; i < sizeof suffixes / sizeof suffixes[0]; i++)
    {
        char text[TEST_TEXT_SIZE];
        size_t length = strlen(SAVE_FIXTURE_TEXT);
        memcpy(text, SAVE_FIXTURE_TEXT, length);
        memcpy(text + length, suffixes[i], strlen(suffixes[i]) + 1);
        expect_text_rejected(text, length + strlen(suffixes[i]));
    }
    char unterminated[TEST_TEXT_SIZE];
    size_t length = strlen(SAVE_FIXTURE_TEXT) - 1;
    memcpy(unterminated, SAVE_FIXTURE_TEXT, length);
    unterminated[length] = ' ';
    expect_text_rejected(unterminated, length + 1);
}

static void test_parse_saved_game_rejects_text_over_the_limit(void)
{
    char text[TEST_TEXT_SIZE + 1];
    size_t length = strlen(SAVE_FIXTURE_TEXT);
    memcpy(text, SAVE_FIXTURE_TEXT, length);
    memset(text + length, ' ', sizeof text - length);
    expect_text_rejected(text, TETONOR_SAVED_GAME_TEXT_MAX + 1);
    expect_text_rejected(text, TETONOR_SAVED_GAME_TEXT_MAX);
    memset(text, 'a', sizeof text);
    expect_text_rejected(text, TETONOR_SAVED_GAME_TEXT_MAX + 1);
}

static void test_parse_saved_game_rejects_an_embedded_nul(void)
{
    char text[TEST_TEXT_SIZE];
    size_t length = strlen(SAVE_FIXTURE_TEXT);
    memcpy(text, SAVE_FIXTURE_TEXT, length + 1);
    text[TEST_NUL_POSITION] = '\0';
    expect_text_rejected(text, length);
    memcpy(text, SAVE_FIXTURE_TEXT, length + 1);
    text[length - 1] = '\0';
    expect_text_rejected(text, length);
}

static const TextEdit malformed_edits[] = {
    {"TETONOR-SAVE 1", "TETONOR-SAVE 2"},
    {"TETONOR-SAVE 1", "TETONOR-SAVE 0"},
    {"TETONOR-SAVE 1\n", "TETONOR-SAVE 10\n"},
    {"TETONOR-SAVE 1", "TETONOR-SAVE  1"},
    {"TETONOR-SAVE 1", "TETONOR-SAVE"},
    {"TETONOR-SAVE 1", "tetonor-save 1"},
    {"TETONOR-SAVE 1", "TETONOR-STATS 1"},
    {"TETONOR-SAVE 1", " TETONOR-SAVE 1"},
    {"difficulty 0", "dificulty 0"},
    {"difficulty 0", "difficulty"},
    {"difficulty 0", "difficulty  0"},
    {"difficulty 0", "difficulty +0"},
    {"difficulty 0", "difficulty -0"},
    {"difficulty 0", "difficulty 0 "},
    {"difficulty 0", "difficulty\t0"},
    {"difficulty 0", "difficulty 0x"},
    {"elapsed 754", "elapsed -1"},
    {"elapsed 754", "elapsed +754"},
    {"elapsed 754", "elapsed 7 54"},
    {"elapsed 754", "elapsed 7.5"},
    {"elapsed 754", "elapsed 0x2f2"},
    {"elapsed 754", "elapsed "},
    {"elapsed 754", "elapsed 99999999999999999999"},
    {"moves 7", "moves"},
    {"moves 7", "moves -7"},
    {"moves 7", "moves 00000000007"},
    {"moves 7", "moves 4294967303"},
    {"moves 7", "moves 2147483648"},
    {"moves 7", "moves 9999999999"},
    {"result pending", "result PENDING"},
    {"result pending", "result correct"},
    {"result pending", "result"},
    {"result pending", "result pending "},
    {"matrix 3 2 7", "matrix 3 2"},
    {"6 8\nstrip", "6\nstrip"},
    {"6 8\nstrip", "6 8 9\nstrip"},
    {"6 8\nstrip", "6  8\nstrip"},
    {"6 8\nstrip", "6 8 \nstrip"},
    {"6 8\nstrip", "6 -\nstrip"},
    {"6 8\nstrip", "6 x\nstrip"},
    {"6 8\nstrip", "6 +8\nstrip"},
    {"matrix 3 2", "matrix 3 3"},
    {"matrix 3 2", "matrix 3 -2"},
    {"matrix 3 2", "matrix\t3 2"},
    {"strip 1 - 2", "strip 1 - 3"},
    {"strip 1 - 2", "strip 1 + 2"},
    {"strip 1 - 2", "strip 1 -- 2"},
    {"4 4 - 6", "4 4 7 6"},
    {"strip 1", "strip"},
    {"solution 1 1", "solution 2 1"},
    {"solution 1 1", "solution 1 -"},
    {"solution 1 1 2 2 3 3 4 4 5 6 7 8 9 10 11 12\nsolution-groups",
     "solution 1 1 2 2 3 3 4 4 5 6 7 8 9 10 11 13\nsolution-groups"},
    {"solution 1 1 2 2 3 3 4 4 5 6 7 8 9 10 11 12\nsolution-groups",
     "solution 1 1 2 2 3 3 4 4 5 6 7 8 9 10 12 11\nsolution-groups"},
    {"solution 1 1 2 2 3 3 4 4 5 6 7 8 9 10 11 12\nsolution-groups",
     "solution 1 1 2 2 3 3 4 4 5 6 7 8 9 10 11\nsolution-groups"},
    {"solution-groups 1+2", "solution-groups 1-2"},
    {"solution-groups 1+2", "solution-groups 1*2"},
    {"solution-groups 1+2", "solution-groups 1X2"},
    {"solution-groups 1+2", "solution-groups 12"},
    {"solution-groups 1+2", "solution-groups 1+"},
    {"solution-groups 1+2", "solution-groups +2"},
    {"solution-groups 1+2", "solution-groups ---"},
    {"solution-groups 1+2", "solution-groups 1+3"},
    {"solution-groups 1+2", "solution-groups 1x2"},
    {"1+2 1x2 3+4", "1+2 1x2"},
    {"2+4 2x4", "2+4 2x4 2x4"},
    {"slots - 1", "slots 5 1"},
    {"slots - 1", "slots - 30"},
    {"slots - 1", "slots - 1000000000000"},
    {"slots - 1", "slots -1 1"},
    {"slots - 1", "slots - 4294967297"},
    {"12--", "4294967308--"},
    {"12--", "12-4294967308"},
    {"matrix 3 2", "matrix 4294967299 2"},
    {"solution 1 1", "solution 4294967297 1"},
    {"slots - 1", "slots - x"},
    {"slots - 1", "slots - +"},
    {"slots - 1", "slots  - 1"},
    {"slots - 1", "slots - 01x"},
    {"\ngroups 1+2", "\ngroups 1+30"},
    {"\ngroups 1+2", "\ngroups 30+2"},
    {"\ngroups 1+2", "\ngroups 1*2"},
    {"\ngroups 1+2", "\ngroups 1+-2"},
    {"\ngroups 1+2", "\ngroups 1+2x"},
    {"\ngroups 1+2", "\ngroups 1+2+"},
    {"\ngroups 1+2", "\ngroups 12"},
    {"\ngroups 1+2", "\ngroups"},
    {"\ngroups 1+2", "\ngroup 1+2"},
    {"12--", "12-"},
    {"12--", "12---"},
    {"12--", "-2--"},
    {"12--", "12-x"},
    {"12--", "99--"},
    {" --- --- --- --- --- --- --- --- --- --- ---\n", "\n"},
    {"-x-", "-*-"},
    {"-x-", "-X-"},
    {"-x-", "xx-"},
    {"-x-", "-xx"},
    {"-x-", "--x"},
    {"moves 7\n", ""},
    {"moves 7\n", "moves 7\nmoves 7\n"},
    {"elapsed 754\nmoves 7\n", "moves 7\nelapsed 754\n"},
    {"difficulty 0\nelapsed 754\n", "elapsed 754\ndifficulty 0\n"},
    {"strip 1 - 2 2 3 - 4 4 - 6 7 8 - 10 11 12\nsolution 1 1 2 2 3 3 4 4 5 6 7 "
     "8 9 10 11 12\n",
     "solution 1 1 2 2 3 3 4 4 5 6 7 8 9 10 11 12\nstrip 1 - 2 2 3 - 4 4 - 6 7 "
     "8 - 10 11 12\n"},
    {"moves 7\n", "moves 7\r"},
    {"moves 7\n", "moves 7\n\n"},
    {"moves 7\n", "moves 7\n\r\n"},
    {"moves 7\n", "moves 7 \n"},
    {"moves 7\n", " moves 7\n"},
    {"moves 7\n", "moves\t7\n"},
};

static void test_parse_saved_game_rejects_every_malformed_edit(void)
{
    for (size_t i = 0; i < sizeof malformed_edits / sizeof malformed_edits[0];
         i++)
    {
        char text[TEST_TEXT_SIZE];
        size_t length = 0;
        apply_edit(&malformed_edits[i], text, sizeof text, &length);
        expect_text_rejected(text, length);
    }
}

static const TextEdit inconsistent_edits[] = {
    {"difficulty 0", "difficulty 1"},
    {"difficulty 0", "difficulty 2"},
    {"difficulty 0", "difficulty 3"},
    {"difficulty 0", "difficulty 4294967296"},
    {"elapsed 754", "elapsed 360000"},
    {"elapsed 754", "elapsed 4294967296"},
    {"moves 7", "moves 6"},
    {"moves 7", "moves 0"},
    {"result pending", "result wrong"},
    {"slots - 1", "slots 1 1"},
    {"slots - 1 - - - - - - 5", "slots - 1 - - - - - - 5 5"},
    {"groups 1+2 -x- 12--", "groups 1+2 -x- 12-30"},
    {"strip 1 - 2", "strip - - 2"},
    {"matrix 3 2 7 12", "matrix 3 2 7 13"},
};

static void test_parse_saved_game_rejects_values_that_break_a_rule(void)
{
    for (size_t i = 0;
         i < sizeof inconsistent_edits / sizeof inconsistent_edits[0]; i++)
    {
        char text[TEST_TEXT_SIZE];
        size_t length = 0;
        apply_edit(&inconsistent_edits[i], text, sizeof text, &length);
        expect_text_rejected(text, length);
    }
}

static void test_restore_saved_game_restores_the_board(void)
{
    SavedGame saved;
    build_saved_game_fixture(&saved);
    NumberGeneration other;
    build_fixture_puzzle(&other, NULL, 0);
    Board board;
    init_board(&board, &other);
    GameTimer timer;
    start_game_timer(&timer, TEST_OTHER_NOW);
    NumberGeneration* puzzle =
        restore_saved_game(&saved, &board, &timer, TEST_NOW);
    ASSERT_NOT_NULL(puzzle);
    Board fresh;
    init_board(&fresh, puzzle);
    bool same_puzzle = number_generations_equal(puzzle, &saved.puzzle);
    bool owned       = puzzle != &saved.puzzle && board.puzzle == puzzle;
    bool same_start =
        board.cursor == fresh.cursor && board.focus == fresh.focus &&
        board.group_cursor == fresh.group_cursor &&
        board.group_field == fresh.group_field &&
        memcmp(&board.palette, &fresh.palette, sizeof board.palette) == 0;
    bool same_entries = true;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        same_entries =
            same_entries &&
            board.slots[i].editable == fresh.slots[i].editable &&
            board.slots[i].value == saved.entries.slot_values[i] &&
            operand_groups_equal(&board.groups[i], &saved.entries.groups[i]);
    }
    int moves          = board.move_count;
    int history        = board.history.count;
    BoardResult result = board.result;
    free_number_generation(puzzle);
    ASSERT_TRUE(same_puzzle);
    ASSERT_TRUE(owned);
    ASSERT_TRUE(same_start);
    ASSERT_TRUE(same_entries);
    ASSERT_EQ_INT(moves, SAVE_FIXTURE_MOVES);
    ASSERT_EQ_INT(history, 0);
    ASSERT_EQ_INT(result, BOARD_RESULT_PENDING);
}

static void test_restore_saved_game_recomputes_the_slot_usages(void)
{
    SavedGame saved;
    build_empty_saved_game(&saved, DIFFICULTY_EASY);
    fill_saved_game_with_solution(&saved);
    Board board;
    GameTimer timer;
    NumberGeneration* puzzle =
        restore_saved_game(&saved, &board, &timer, TEST_NOW);
    ASSERT_NOT_NULL(puzzle);
    int strip[TETONOR_CELL_COUNT];
    SlotUsage expected[TETONOR_CELL_COUNT];
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        strip[i] = board.slots[i].editable ? saved.entries.slot_values[i]
                                           : puzzle->available_array[i];
    }
    compute_slot_usages(strip, saved.entries.groups, expected);
    bool same = true;
    bool used = false;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        same = same && board.slot_usages[i] == expected[i];
        used = used || expected[i] == SLOT_USAGE_TWICE;
    }
    free_number_generation(puzzle);
    ASSERT_TRUE(same);
    ASSERT_TRUE(used);
}

static void test_restore_saved_game_restores_a_wrong_result(void)
{
    SavedGame saved;
    build_empty_saved_game(&saved, DIFFICULTY_MEDIUM);
    fill_saved_game_with_wrong_answer(&saved);
    Board board;
    GameTimer timer;
    NumberGeneration* puzzle =
        restore_saved_game(&saved, &board, &timer, TEST_NOW);
    ASSERT_NOT_NULL(puzzle);
    BoardResult result = board.result;
    bool locked        = is_board_locked(&board);
    free_number_generation(puzzle);
    ASSERT_EQ_INT(result, BOARD_RESULT_WRONG);
    ASSERT_FALSE(locked);
}

static void test_restore_saved_game_restores_the_timer(void)
{
    SavedGame saved;
    build_saved_game_fixture(&saved);
    NumberGeneration other;
    build_fixture_puzzle(&other, NULL, 0);
    Board board;
    init_board(&board, &other);
    GameTimer timer;
    start_game_timer(&timer, TEST_OTHER_NOW);
    pause_game_timer(&timer, TEST_OTHER_NOW + 50);
    NumberGeneration* puzzle =
        restore_saved_game(&saved, &board, &timer, TEST_NOW);
    ASSERT_NOT_NULL(puzzle);
    free_number_generation(puzzle);
    ASSERT_EQ_INT(timer.elapsed_seconds, SAVE_FIXTURE_ELAPSED);
    ASSERT_TRUE(timer.running);
    ASSERT_FALSE(timer.paused);
    ASSERT_TRUE(timer.last_tick == TEST_NOW);
    ASSERT_FALSE(update_game_timer(&timer, BOARD_RESULT_PENDING, TEST_NOW + 6));
    ASSERT_EQ_INT(timer.elapsed_seconds, SAVE_FIXTURE_ELAPSED + 6);
}

static void test_restore_saved_game_timer_bounds(void)
{
    SavedGame saved;
    build_saved_game_fixture(&saved);
    const int32_t values[] = {0, TETONOR_TIMER_MAX_SECONDS};
    for (size_t i = 0; i < sizeof values / sizeof values[0]; i++)
    {
        saved.elapsed_seconds = values[i];
        Board board;
        GameTimer timer;
        NumberGeneration* puzzle =
            restore_saved_game(&saved, &board, &timer, TEST_NOW);
        ASSERT_NOT_NULL(puzzle);
        free_number_generation(puzzle);
        ASSERT_EQ_INT(timer.elapsed_seconds, values[i]);
    }
}

static void test_restore_saved_game_puzzle_is_independent_of_the_save(void)
{
    SavedGame saved;
    build_saved_game_fixture(&saved);
    Board board;
    GameTimer timer;
    NumberGeneration* puzzle =
        restore_saved_game(&saved, &board, &timer, TEST_NOW);
    ASSERT_NOT_NULL(puzzle);
    saved.puzzle.available_array[0] = 99;
    saved.puzzle.sum_matrix[0][0]   = 99;
    SavedGame expected;
    build_saved_game_fixture(&expected);
    bool unchanged = number_generations_equal(puzzle, &expected.puzzle);
    free_number_generation(puzzle);
    ASSERT_TRUE(unchanged);
}

static void test_restore_saved_game_invalid_save_changes_nothing(void)
{
    SavedGame saved;
    build_saved_game_fixture(&saved);
    saved.elapsed_seconds = -1;
    NumberGeneration other;
    build_fixture_puzzle(&other, NULL, 0);
    Board board;
    init_board(&board, &other);
    GameTimer timer;
    start_game_timer(&timer, TEST_OTHER_NOW);
    unsigned char board_before[sizeof board];
    unsigned char timer_before[sizeof timer];
    remember_bytes(&board, sizeof board, board_before);
    remember_bytes(&timer, sizeof timer, timer_before);
    ASSERT_NULL(restore_saved_game(&saved, &board, &timer, TEST_NOW));
    ASSERT_TRUE(bytes_unchanged(&board, sizeof board, board_before));
    ASSERT_TRUE(bytes_unchanged(&timer, sizeof timer, timer_before));
    build_saved_game_fixture(&saved);
    saved.entries.result = BOARD_RESULT_CORRECT;
    ASSERT_NULL(restore_saved_game(&saved, &board, &timer, TEST_NOW));
    ASSERT_TRUE(bytes_unchanged(&board, sizeof board, board_before));
    ASSERT_TRUE(bytes_unchanged(&timer, sizeof timer, timer_before));
}

/* capture, serialize, parse and restore must hand back the same game. */
static void check_round_trip(const Board* board, int difficulty)
{
    SavedGame saved;
    ASSERT_TRUE(capture_saved_game(board, TEST_ELAPSED, difficulty, &saved));
    ASSERT_TRUE(is_saved_game_valid(&saved));
    char text[TEST_TEXT_SIZE];
    ASSERT_TRUE(serialize_saved_game(&saved, text, sizeof text));
    SavedGame parsed;
    ASSERT_TRUE(parse_saved_game(text, strlen(text), &parsed));
    ASSERT_TRUE(saved_games_equal(&saved, &parsed));
    Board restored;
    GameTimer timer;
    NumberGeneration* puzzle =
        restore_saved_game(&parsed, &restored, &timer, TEST_NOW);
    ASSERT_NOT_NULL(puzzle);
    bool same_puzzle  = number_generations_equal(puzzle, board->puzzle);
    bool same_entries = boards_have_same_entries(&restored, board);
    int history       = restored.history.count;
    free_number_generation(puzzle);
    ASSERT_TRUE(same_puzzle);
    ASSERT_TRUE(same_entries);
    ASSERT_EQ_INT(history, 0);
    ASSERT_EQ_INT(timer.elapsed_seconds, TEST_ELAPSED);
}

static void fill_every_box(Board* board)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (board->slots[i].editable)
        {
            place_slot_focus(board, i);
            press_digit(board, TEST_EDIT_DIGIT);
        }
    }
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        place_group_focus(board, cell, GROUP_FIELD_SMALLER);
        press_digit(board, TEST_GROUP_SMALLER);
        place_group_focus(board, cell, GROUP_FIELD_OPERATION);
        press_operation(board, GROUP_OPERATION_ADD);
        place_group_focus(board, cell, GROUP_FIELD_GREATER);
        press_digit(board, TEST_GROUP_GREATER);
    }
}

static void submit(Board* board)
{
    board->focus = BOARD_FOCUS_SUBMIT;
    press(board, BOARD_INPUT_ACTIVATE);
}

/* Guards the plan's risk R1: every state a real game reaches must be savable
 * and must come back unchanged. */
static void run_scripted_game(int difficulty, unsigned seed)
{
    srand(seed);
    NumberGeneration* puzzle = generate_random_numbers(difficulty);
    ASSERT_NOT_NULL(puzzle);
    Board board;
    init_board(&board, puzzle);
    check_round_trip(&board, difficulty);
    press_digit(&board, 1);
    press_digit(&board, 2);
    check_round_trip(&board, difficulty);
    press(&board, BOARD_INPUT_DELETE);
    check_round_trip(&board, difficulty);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    press_digit(&board, 7);
    check_round_trip(&board, difficulty);
    place_group_focus(&board, 0, GROUP_FIELD_SMALLER);
    press_digit(&board, 3);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    press_operation(&board, GROUP_OPERATION_ADD);
    press(&board, BOARD_INPUT_MOVE_RIGHT);
    press_digit(&board, 4);
    check_round_trip(&board, difficulty);
    ASSERT_TRUE(undo_board_move(&board));
    ASSERT_TRUE(undo_board_move(&board));
    check_round_trip(&board, difficulty);
    fill_every_box(&board);
    check_round_trip(&board, difficulty);
    submit(&board);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_WRONG);
    check_round_trip(&board, difficulty);
    place_slot_focus(&board, board.cursor);
    press(&board, BOARD_INPUT_DELETE);
    ASSERT_EQ_INT(board.result, BOARD_RESULT_PENDING);
    check_round_trip(&board, difficulty);
    press_digit(&board, TEST_EDIT_DIGIT);
    submit(&board);
    check_round_trip(&board, difficulty);
    ASSERT_TRUE(undo_board_move(&board));
    check_round_trip(&board, difficulty);
    free_number_generation(puzzle);
}

static void test_saved_game_round_trips_scripted_games_at_every_level(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
        {
            run_scripted_game(test_levels[level], seed);
        }
    }
}

static void test_saved_game_round_trips_the_golden_text_through_a_board(void)
{
    NumberGeneration puzzle;
    Board board;
    start_board(&puzzle, &board, DIFFICULTY_EASY);
    type_golden_entries(&board);
    SavedGame saved;
    ASSERT_TRUE(capture_saved_game(&board, SAVE_FIXTURE_ELAPSED,
                                   DIFFICULTY_EASY, &saved));
    char text[TEST_TEXT_SIZE];
    ASSERT_TRUE(serialize_saved_game(&saved, text, sizeof text));
    ASSERT_TRUE(strcmp(text, SAVE_FIXTURE_TEXT) == 0);
}

int main(void)
{
    RUN_TEST(test_capture_saved_game_copies_the_game);
    RUN_TEST(test_capture_saved_game_keeps_the_given_difficulty_and_time);
    RUN_TEST(test_capture_saved_game_is_a_deep_copy);
    RUN_TEST(test_capture_saved_game_takes_the_move_count_of_the_board);
    RUN_TEST(test_capture_saved_game_accepts_a_wrong_answer);
    RUN_TEST(test_capture_saved_game_locked_board_fails_untouched);
    RUN_TEST(test_capture_saved_game_without_a_puzzle_fails_untouched);
    RUN_TEST(test_capture_saved_game_invalid_game_fails_untouched);
    RUN_TEST(test_is_saved_game_valid_accepts_the_fixture);
    RUN_TEST(test_is_saved_game_valid_accepts_every_difficulty);
    RUN_TEST(test_is_saved_game_valid_difficulty_must_be_a_level);
    RUN_TEST(test_is_saved_game_valid_difficulty_must_match_the_hidden_count);
    RUN_TEST(test_is_saved_game_valid_elapsed_bounds);
    RUN_TEST(test_is_saved_game_valid_move_count_bounds);
    RUN_TEST(test_is_saved_game_valid_solution_range_per_level);
    RUN_TEST(test_is_saved_game_valid_solution_must_be_sorted);
    RUN_TEST(test_is_saved_game_valid_hidden_count_must_match_the_level);
    RUN_TEST(test_is_saved_game_valid_visible_value_must_be_the_solution);
    RUN_TEST(test_is_saved_game_valid_solution_must_solve_the_matrix);
    RUN_TEST(test_is_saved_game_valid_entry_only_in_a_hidden_slot);
    RUN_TEST(test_is_saved_game_valid_slot_value_bounds);
    RUN_TEST(test_is_saved_game_valid_group_number_bounds);
    RUN_TEST(test_is_saved_game_valid_group_operation_must_be_known);
    RUN_TEST(test_is_saved_game_valid_result_pending_or_wrong_only);
    RUN_TEST(test_is_saved_game_valid_wrong_result_needs_a_wrong_answer);
    RUN_TEST(test_is_saved_game_valid_wrong_result_needs_every_box_filled);
    RUN_TEST(test_is_saved_game_valid_pending_may_hold_a_correct_answer);
    RUN_TEST(test_is_saved_game_valid_wrong_result_with_a_correct_answer);
    RUN_TEST(test_is_saved_game_valid_wrong_result_judges_typed_slots);
    RUN_TEST(test_is_saved_game_valid_moves_cover_the_filled_boxes);
    RUN_TEST(test_is_saved_game_valid_moves_cover_a_full_board);
    RUN_TEST(test_serialize_saved_game_writes_the_documented_text);
    RUN_TEST(test_serialize_saved_game_writes_the_wrong_result_word);
    RUN_TEST(test_serialize_saved_game_every_smaller_size_fails);
    RUN_TEST(test_serialize_saved_game_exact_size_fits);
    RUN_TEST(test_serialize_saved_game_widest_game_fits_the_file_limit);
    RUN_TEST(test_parse_saved_game_round_trips_the_fixture);
    RUN_TEST(test_parse_saved_game_round_trips_every_difficulty);
    RUN_TEST(test_parse_saved_game_accepts_crlf_line_ends);
    RUN_TEST(test_parse_saved_game_accepts_a_last_line_without_line_end);
    RUN_TEST(test_parse_saved_game_reads_only_the_given_length);
    RUN_TEST(test_parse_saved_game_accepts_the_value_boundaries);
    RUN_TEST(test_parse_saved_game_rejects_empty_and_header_only_text);
    RUN_TEST(test_parse_saved_game_rejects_every_truncation);
    RUN_TEST(test_parse_saved_game_rejects_trailing_bytes);
    RUN_TEST(test_parse_saved_game_rejects_text_over_the_limit);
    RUN_TEST(test_parse_saved_game_rejects_an_embedded_nul);
    RUN_TEST(test_parse_saved_game_rejects_every_malformed_edit);
    RUN_TEST(test_parse_saved_game_rejects_values_that_break_a_rule);
    RUN_TEST(test_restore_saved_game_restores_the_board);
    RUN_TEST(test_restore_saved_game_recomputes_the_slot_usages);
    RUN_TEST(test_restore_saved_game_restores_a_wrong_result);
    RUN_TEST(test_restore_saved_game_restores_the_timer);
    RUN_TEST(test_restore_saved_game_timer_bounds);
    RUN_TEST(test_restore_saved_game_puzzle_is_independent_of_the_save);
    RUN_TEST(test_restore_saved_game_invalid_save_changes_nothing);
    RUN_TEST(test_saved_game_round_trips_scripted_games_at_every_level);
    RUN_TEST(test_saved_game_round_trips_the_golden_text_through_a_board);
    return test_harness_result();
}
