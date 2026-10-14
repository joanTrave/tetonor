#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../test_harness.h"
#include "fixture_puzzle.h"
#include "../../src/game/answer_check.h"

#define TEST_SEED_COUNT 50
#define TEST_DIFFICULTY 1
#define TEST_FIRST_HIDDEN 2
#define TEST_SECOND_HIDDEN 5
#define TEST_OUT_OF_RANGE_ABOVE (TETONOR_MAX_VALUE + 1)
#define TEST_OUT_OF_RANGE_BELOW (-5)
#define TEST_ALTERED_CELLS 4

static const int fixture_hidden[] = {TEST_FIRST_HIDDEN, TEST_SECOND_HIDDEN, 6,
                                     15};

/* Fixture F-B: the first two pairs are (1,4) and (2,3) instead of (1,2) and
 * (3,4). The strip is the same, so the player may use either assignment of the
 * two pairs to cells 0 and 2. */
static const int fixture_b_sums[TEST_ALTERED_CELLS] = {5, 4, 5, 6};

/* Fixture F-D: the first two pairs are (1,2) and (3,3). The strip changes at
 * index 4 (4 becomes 3). */
static const int fixture_d_sums[TEST_ALTERED_CELLS] = {3, 2, 6, 9};
static const int fixture_d_strip_index              = 4;
static const int fixture_d_strip_value              = 3;

static OperandGroup make_group(int smaller, GroupOperation operation,
                               int greater)
{
    OperandGroup group = {smaller, operation, greater};
    return group;
}

static void build_puzzle(NumberGeneration* puzzle)
{
    build_fixture_puzzle(puzzle, fixture_hidden,
                         sizeof fixture_hidden / sizeof fixture_hidden[0]);
}

static void set_first_cells(NumberGeneration* puzzle,
                            const int sums[TEST_ALTERED_CELLS])
{
    for (int i = 0; i < TEST_ALTERED_CELLS; i++)
    {
        puzzle->sum_matrix[i / TETONOR_GRID_SIZE][i % TETONOR_GRID_SIZE] =
            sums[i];
    }
}

static void build_fixture_b(NumberGeneration* puzzle,
                            OperandGroup groups[TETONOR_CELL_COUNT])
{
    build_puzzle(puzzle);
    set_first_cells(puzzle, fixture_b_sums);
    memcpy(groups, fixture_groups, sizeof fixture_groups);
    groups[0] = make_group(1, GROUP_OPERATION_ADD, 4);
    groups[1] = make_group(1, GROUP_OPERATION_MULTIPLY, 4);
    groups[2] = make_group(2, GROUP_OPERATION_ADD, 3);
    groups[3] = make_group(2, GROUP_OPERATION_MULTIPLY, 3);
}

static void build_fixture_d(NumberGeneration* puzzle,
                            OperandGroup groups[TETONOR_CELL_COUNT],
                            int strip[TETONOR_CELL_COUNT])
{
    build_puzzle(puzzle);
    set_first_cells(puzzle, fixture_d_sums);
    memcpy(strip, fixture_strip, sizeof fixture_strip);
    strip[fixture_d_strip_index] = fixture_d_strip_value;
    memcpy(groups, fixture_groups, sizeof fixture_groups);
    groups[2] = make_group(3, GROUP_OPERATION_ADD, 3);
    groups[3] = make_group(3, GROUP_OPERATION_MULTIPLY, 3);
}

static void test_is_answer_correct_solution_is_correct(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    ASSERT_TRUE(is_answer_correct(&puzzle, fixture_strip, fixture_groups));
}

static void test_is_answer_correct_one_wrong_hidden_entry_is_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    for (size_t i = 0; i < sizeof fixture_hidden / sizeof fixture_hidden[0];
         i++)
    {
        int strip[TETONOR_CELL_COUNT];
        memcpy(strip, fixture_strip, sizeof strip);
        int index    = fixture_hidden[i];
        strip[index] = (strip[index] + 1) % (TETONOR_MAX_VALUE + 1);
        ASSERT_FALSE(is_answer_correct(&puzzle, strip, fixture_groups));
    }
}

static void test_is_answer_correct_any_single_changed_position_is_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        int strip[TETONOR_CELL_COUNT];
        memcpy(strip, fixture_strip, sizeof strip);
        strip[i] = (strip[i] + 1) % (TETONOR_MAX_VALUE + 1);
        ASSERT_FALSE(is_answer_correct(&puzzle, strip, fixture_groups));
    }
}

static void
test_is_answer_correct_sorted_strip_not_matching_operands_is_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    int strip[TETONOR_CELL_COUNT];
    memcpy(strip, fixture_strip, sizeof strip);
    strip[TETONOR_CELL_COUNT - 1] = TETONOR_MAX_VALUE - 1;
    ASSERT_FALSE(is_answer_correct(&puzzle, strip, fixture_groups));
}

static void test_is_answer_correct_swapped_entries_is_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    int strip[TETONOR_CELL_COUNT];
    memcpy(strip, fixture_strip, sizeof strip);
    strip[TEST_FIRST_HIDDEN]  = fixture_strip[TEST_SECOND_HIDDEN];
    strip[TEST_SECOND_HIDDEN] = fixture_strip[TEST_FIRST_HIDDEN];
    ASSERT_FALSE(is_answer_correct(&puzzle, strip, fixture_groups));
}

static void
test_is_answer_correct_unsorted_strip_with_right_values_is_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    int strip[TETONOR_CELL_COUNT];
    memcpy(strip, fixture_strip, sizeof strip);
    strip[0] = fixture_strip[1];
    strip[1] = fixture_strip[0];
    ASSERT_FALSE(is_answer_correct(&puzzle, strip, fixture_groups));
}

static void test_is_answer_correct_unfilled_hidden_slot_is_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    ASSERT_FALSE(
        is_answer_correct(&puzzle, puzzle.available_array, fixture_groups));
}

static void test_is_answer_correct_out_of_range_strip_value_is_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    int strip[TETONOR_CELL_COUNT];
    memcpy(strip, fixture_strip, sizeof strip);
    strip[TETONOR_CELL_COUNT - 1] = TEST_OUT_OF_RANGE_ABOVE;
    ASSERT_FALSE(is_answer_correct(&puzzle, strip, fixture_groups));
    memcpy(strip, fixture_strip, sizeof strip);
    strip[0] = TEST_OUT_OF_RANGE_BELOW;
    ASSERT_FALSE(is_answer_correct(&puzzle, strip, fixture_groups));
}

static void test_is_answer_correct_does_not_modify_its_inputs(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    NumberGeneration puzzle_before = puzzle;
    int strip[TETONOR_CELL_COUNT];
    OperandGroup groups[TETONOR_CELL_COUNT];
    memcpy(strip, fixture_strip, sizeof strip);
    memcpy(groups, fixture_groups, sizeof groups);
    ASSERT_TRUE(is_answer_correct(&puzzle, strip, groups));
    ASSERT_TRUE(memcmp(&puzzle, &puzzle_before, sizeof puzzle) == 0);
    ASSERT_TRUE(memcmp(strip, fixture_strip, sizeof strip) == 0);
    ASSERT_TRUE(memcmp(groups, fixture_groups, sizeof groups) == 0);
}

static void test_is_answer_correct_wrong_operation_in_one_group_is_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        OperandGroup groups[TETONOR_CELL_COUNT];
        memcpy(groups, fixture_groups, sizeof groups);
        groups[i].operation = groups[i].operation == GROUP_OPERATION_ADD
                                  ? GROUP_OPERATION_MULTIPLY
                                  : GROUP_OPERATION_ADD;
        ASSERT_FALSE(is_answer_correct(&puzzle, fixture_strip, groups));
    }
}

static void test_is_answer_correct_wrong_number_in_one_group_is_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        OperandGroup groups[TETONOR_CELL_COUNT];
        memcpy(groups, fixture_groups, sizeof groups);
        groups[i].greater += 1;
        ASSERT_FALSE(is_answer_correct(&puzzle, fixture_strip, groups));
        memcpy(groups, fixture_groups, sizeof groups);
        groups[i].smaller += 1;
        ASSERT_FALSE(is_answer_correct(&puzzle, fixture_strip, groups));
    }
}

static void test_is_answer_correct_swapped_group_numbers_are_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        OperandGroup groups[TETONOR_CELL_COUNT];
        memcpy(groups, fixture_groups, sizeof groups);
        groups[i].smaller = fixture_groups[i].greater;
        groups[i].greater = fixture_groups[i].smaller;
        ASSERT_FALSE(is_answer_correct(&puzzle, fixture_strip, groups));
    }
}

static void test_is_answer_correct_groups_in_swapped_cells_are_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    OperandGroup groups[TETONOR_CELL_COUNT];
    memcpy(groups, fixture_groups, sizeof groups);
    groups[0] = fixture_groups[2];
    groups[2] = fixture_groups[0];
    ASSERT_FALSE(is_answer_correct(&puzzle, fixture_strip, groups));
}

static void test_is_answer_correct_empty_group_field_is_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        OperandGroup groups[TETONOR_CELL_COUNT];
        memcpy(groups, fixture_groups, sizeof groups);
        groups[i].smaller = TETONOR_EMPTY_ENTRY;
        ASSERT_FALSE(is_answer_correct(&puzzle, fixture_strip, groups));
        memcpy(groups, fixture_groups, sizeof groups);
        groups[i].operation = GROUP_OPERATION_NONE;
        ASSERT_FALSE(is_answer_correct(&puzzle, fixture_strip, groups));
        memcpy(groups, fixture_groups, sizeof groups);
        groups[i].greater = TETONOR_EMPTY_ENTRY;
        ASSERT_FALSE(is_answer_correct(&puzzle, fixture_strip, groups));
    }
}

static void test_is_answer_correct_all_groups_empty_is_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    OperandGroup groups[TETONOR_CELL_COUNT];
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        clear_operand_group(&groups[i]);
    }
    ASSERT_FALSE(is_answer_correct(&puzzle, fixture_strip, groups));
}

static void test_is_answer_correct_out_of_range_group_number_is_wrong(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    OperandGroup groups[TETONOR_CELL_COUNT];
    memcpy(groups, fixture_groups, sizeof groups);
    groups[0].greater = TEST_OUT_OF_RANGE_ABOVE;
    ASSERT_FALSE(is_answer_correct(&puzzle, fixture_strip, groups));
    memcpy(groups, fixture_groups, sizeof groups);
    groups[0].smaller = TEST_OUT_OF_RANGE_BELOW;
    ASSERT_FALSE(is_answer_correct(&puzzle, fixture_strip, groups));
}

static void
test_is_answer_correct_accepts_the_generators_assignment_of_f_b(void)
{
    NumberGeneration puzzle;
    OperandGroup groups[TETONOR_CELL_COUNT];
    build_fixture_b(&puzzle, groups);
    ASSERT_TRUE(is_answer_correct(&puzzle, fixture_strip, groups));
}

static void test_is_answer_correct_accepts_an_alternative_valid_solution(void)
{
    NumberGeneration puzzle;
    OperandGroup groups[TETONOR_CELL_COUNT];
    build_fixture_b(&puzzle, groups);
    groups[0] = make_group(2, GROUP_OPERATION_ADD, 3);
    groups[2] = make_group(1, GROUP_OPERATION_ADD, 4);
    ASSERT_TRUE(is_answer_correct(&puzzle, fixture_strip, groups));
}

static void test_is_answer_correct_rejects_valid_groups_reusing_a_number(void)
{
    NumberGeneration puzzle;
    OperandGroup groups[TETONOR_CELL_COUNT];
    build_fixture_b(&puzzle, groups);
    /* 2 x 2 == 4 holds on its own, but the strip has a single 2 to give. */
    groups[1] = make_group(2, GROUP_OPERATION_MULTIPLY, 2);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        ASSERT_TRUE(is_operand_group_valid(
            &groups[i],
            puzzle.sum_matrix[i / TETONOR_GRID_SIZE][i % TETONOR_GRID_SIZE]));
    }
    ASSERT_FALSE(is_answer_correct(&puzzle, fixture_strip, groups));
}

static void test_is_answer_correct_accepts_equal_pair_when_pairs_balance(void)
{
    NumberGeneration puzzle;
    OperandGroup groups[TETONOR_CELL_COUNT];
    int strip[TETONOR_CELL_COUNT];
    build_fixture_d(&puzzle, groups, strip);
    ASSERT_TRUE(is_answer_correct(&puzzle, strip, groups));
}

static void test_is_answer_correct_rejects_unbalanced_pairs_in_f_d(void)
{
    NumberGeneration puzzle;
    OperandGroup groups[TETONOR_CELL_COUNT];
    int strip[TETONOR_CELL_COUNT];
    build_fixture_d(&puzzle, groups, strip);
    groups[0] = make_group(1, GROUP_OPERATION_MULTIPLY, 3);
    groups[1] = make_group(1, GROUP_OPERATION_MULTIPLY, 2);
    groups[2] = make_group(2, GROUP_OPERATION_MULTIPLY, 3);
    groups[3] = make_group(3, GROUP_OPERATION_MULTIPLY, 3);
    /* Every group is valid and every number is used twice: only the rule that
     * each pair is added once and multiplied once is broken. */
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        ASSERT_TRUE(is_operand_group_valid(
            &groups[i],
            puzzle.sum_matrix[i / TETONOR_GRID_SIZE][i % TETONOR_GRID_SIZE]));
    }
    ASSERT_FALSE(is_answer_correct(&puzzle, strip, groups));
}

static void test_is_answer_correct_ignores_the_generators_solution(void)
{
    NumberGeneration puzzle;
    build_puzzle(&puzzle);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        puzzle.solution_array[i] =
            (fixture_strip[i] + 1) % (TETONOR_MAX_VALUE + 1);
        clear_operand_group(&puzzle.solution_groups[i]);
    }
    ASSERT_TRUE(is_answer_correct(&puzzle, fixture_strip, fixture_groups));
}

static NumberGeneration* generate_seeded_puzzle(unsigned seed)
{
    srand(seed);
    return generate_random_numbers(TEST_DIFFICULTY);
}

static void test_is_answer_correct_generated_puzzle_solution_is_correct(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        NumberGeneration* puzzle = generate_seeded_puzzle(seed);
        ASSERT_NOT_NULL(puzzle);
        bool correct = is_answer_correct(puzzle, puzzle->solution_array,
                                         puzzle->solution_groups);
        free_number_generation(puzzle);
        ASSERT_TRUE(correct);
    }
}

static void test_is_answer_correct_generated_puzzle_changed_group_is_wrong(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        NumberGeneration* puzzle = generate_seeded_puzzle(seed);
        ASSERT_NOT_NULL(puzzle);
        OperandGroup groups[TETONOR_CELL_COUNT];
        memcpy(groups, puzzle->solution_groups, sizeof groups);
        groups[0].greater += 1;
        bool correct =
            is_answer_correct(puzzle, puzzle->solution_array, groups);
        free_number_generation(puzzle);
        ASSERT_FALSE(correct);
    }
}

static void test_is_answer_correct_generated_puzzle_changed_entry_is_wrong(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        NumberGeneration* puzzle = generate_seeded_puzzle(seed);
        ASSERT_NOT_NULL(puzzle);
        int strip[TETONOR_CELL_COUNT];
        memcpy(strip, puzzle->solution_array, sizeof strip);
        strip[0] = (strip[0] + 1) % (TETONOR_MAX_VALUE + 1);
        bool correct =
            is_answer_correct(puzzle, strip, puzzle->solution_groups);
        free_number_generation(puzzle);
        ASSERT_FALSE(correct);
    }
}

static void test_is_answer_correct_generated_puzzle_hidden_strip_is_wrong(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        NumberGeneration* puzzle = generate_seeded_puzzle(seed);
        ASSERT_NOT_NULL(puzzle);
        bool correct = is_answer_correct(puzzle, puzzle->available_array,
                                         puzzle->solution_groups);
        free_number_generation(puzzle);
        ASSERT_FALSE(correct);
    }
}

int main(void)
{
    RUN_TEST(test_is_answer_correct_solution_is_correct);
    RUN_TEST(test_is_answer_correct_one_wrong_hidden_entry_is_wrong);
    RUN_TEST(test_is_answer_correct_any_single_changed_position_is_wrong);
    RUN_TEST(
        test_is_answer_correct_sorted_strip_not_matching_operands_is_wrong);
    RUN_TEST(test_is_answer_correct_swapped_entries_is_wrong);
    RUN_TEST(test_is_answer_correct_unsorted_strip_with_right_values_is_wrong);
    RUN_TEST(test_is_answer_correct_unfilled_hidden_slot_is_wrong);
    RUN_TEST(test_is_answer_correct_out_of_range_strip_value_is_wrong);
    RUN_TEST(test_is_answer_correct_does_not_modify_its_inputs);
    RUN_TEST(test_is_answer_correct_wrong_operation_in_one_group_is_wrong);
    RUN_TEST(test_is_answer_correct_wrong_number_in_one_group_is_wrong);
    RUN_TEST(test_is_answer_correct_swapped_group_numbers_are_wrong);
    RUN_TEST(test_is_answer_correct_groups_in_swapped_cells_are_wrong);
    RUN_TEST(test_is_answer_correct_empty_group_field_is_wrong);
    RUN_TEST(test_is_answer_correct_all_groups_empty_is_wrong);
    RUN_TEST(test_is_answer_correct_out_of_range_group_number_is_wrong);
    RUN_TEST(test_is_answer_correct_accepts_the_generators_assignment_of_f_b);
    RUN_TEST(test_is_answer_correct_accepts_an_alternative_valid_solution);
    RUN_TEST(test_is_answer_correct_rejects_valid_groups_reusing_a_number);
    RUN_TEST(test_is_answer_correct_accepts_equal_pair_when_pairs_balance);
    RUN_TEST(test_is_answer_correct_rejects_unbalanced_pairs_in_f_d);
    RUN_TEST(test_is_answer_correct_ignores_the_generators_solution);
    RUN_TEST(test_is_answer_correct_generated_puzzle_solution_is_correct);
    RUN_TEST(test_is_answer_correct_generated_puzzle_changed_group_is_wrong);
    RUN_TEST(test_is_answer_correct_generated_puzzle_changed_entry_is_wrong);
    RUN_TEST(test_is_answer_correct_generated_puzzle_hidden_strip_is_wrong);
    return test_harness_result();
}
