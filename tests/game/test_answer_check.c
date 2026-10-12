#include <stdlib.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/game/answer_check.h"

#define TEST_SEED_COUNT 50
#define TEST_DIFFICULTY 1
#define TEST_FIRST_HIDDEN 2
#define TEST_SECOND_HIDDEN 5

/* Eight pairs (1,2) (3,4) (5,6) (7,8) (9,10) (11,12) (13,14) (0,29), sorted. */
static const int fixture_solution[TETONOR_CELL_COUNT] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 29};
static const int fixture_sums[TETONOR_CELL_COUNT] = {
    3, 2, 7, 12, 11, 30, 15, 56, 19, 90, 23, 132, 27, 182, 29, 0};
static const int fixture_hidden[] = {TEST_FIRST_HIDDEN, TEST_SECOND_HIDDEN, 6,
                                     15};

static void build_puzzle(NumberGeneration* puzzle)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        puzzle->sum_matrix[i / TETONOR_GRID_SIZE][i % TETONOR_GRID_SIZE] =
            fixture_sums[i];
        puzzle->available_array[i] = fixture_solution[i];
        puzzle->solution_array[i]  = fixture_solution[i];
    }
    for (size_t i = 0; i < sizeof fixture_hidden / sizeof fixture_hidden[0];
         i++)
    {
        puzzle->available_array[fixture_hidden[i]] = TETONOR_HIDDEN_NUMBER;
    }
}

static void test_is_answer_correct_solution_is_correct(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    ASSERT_TRUE(is_answer_correct(&puzzle, fixture_solution));
}

static void test_is_answer_correct_one_wrong_hidden_entry_is_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    for (size_t i = 0; i < sizeof fixture_hidden / sizeof fixture_hidden[0];
         i++)
    {
        int answer[TETONOR_CELL_COUNT];
        memcpy(answer, fixture_solution, sizeof answer);
        int index     = fixture_hidden[i];
        answer[index] = (answer[index] + 1) % (TETONOR_MAX_VALUE + 1);
        ASSERT_FALSE(is_answer_correct(&puzzle, answer));
    }
}

static void test_is_answer_correct_any_single_changed_position_is_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        int answer[TETONOR_CELL_COUNT];
        memcpy(answer, fixture_solution, sizeof answer);
        answer[i] = (answer[i] + 1) % (TETONOR_MAX_VALUE + 1);
        ASSERT_FALSE(is_answer_correct(&puzzle, answer));
    }
}

static void test_is_answer_correct_swapped_entries_is_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    int answer[TETONOR_CELL_COUNT];
    memcpy(answer, fixture_solution, sizeof answer);
    answer[TEST_FIRST_HIDDEN]  = fixture_solution[TEST_SECOND_HIDDEN];
    answer[TEST_SECOND_HIDDEN] = fixture_solution[TEST_FIRST_HIDDEN];
    ASSERT_FALSE(is_answer_correct(&puzzle, answer));
}

static void test_is_answer_correct_unfilled_hidden_slot_is_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    ASSERT_FALSE(is_answer_correct(&puzzle, puzzle.available_array));
}

static void test_is_answer_correct_does_not_modify_its_inputs(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    NumberGeneration puzzle_before = puzzle;
    int answer[TETONOR_CELL_COUNT];
    memcpy(answer, fixture_solution, sizeof answer);
    ASSERT_TRUE(is_answer_correct(&puzzle, answer));
    ASSERT_TRUE(memcmp(&puzzle, &puzzle_before, sizeof puzzle) == 0);
    ASSERT_TRUE(memcmp(answer, fixture_solution, sizeof answer) == 0);
}

static void test_is_answer_correct_generated_puzzle_solution_is_correct(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        srand(seed);
        NumberGeneration* puzzle = generate_random_numbers(TEST_DIFFICULTY);
        ASSERT_NOT_NULL(puzzle);
        bool correct = is_answer_correct(puzzle, puzzle->solution_array);
        free_number_generation(puzzle);
        ASSERT_TRUE(correct);
    }
}

static void test_is_answer_correct_generated_puzzle_changed_entry_is_wrong(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        srand(seed);
        NumberGeneration* puzzle = generate_random_numbers(TEST_DIFFICULTY);
        ASSERT_NOT_NULL(puzzle);
        int answer[TETONOR_CELL_COUNT];
        memcpy(answer, puzzle->solution_array, sizeof answer);
        answer[0]    = (answer[0] + 1) % (TETONOR_MAX_VALUE + 1);
        bool correct = is_answer_correct(puzzle, answer);
        free_number_generation(puzzle);
        ASSERT_FALSE(correct);
    }
}

int main(void)
{
    RUN_TEST(test_is_answer_correct_solution_is_correct);
    RUN_TEST(test_is_answer_correct_one_wrong_hidden_entry_is_wrong);
    RUN_TEST(test_is_answer_correct_any_single_changed_position_is_wrong);
    RUN_TEST(test_is_answer_correct_swapped_entries_is_wrong);
    RUN_TEST(test_is_answer_correct_unfilled_hidden_slot_is_wrong);
    RUN_TEST(test_is_answer_correct_does_not_modify_its_inputs);
    RUN_TEST(test_is_answer_correct_generated_puzzle_solution_is_correct);
    RUN_TEST(test_is_answer_correct_generated_puzzle_changed_entry_is_wrong);
    return test_harness_result();
}
