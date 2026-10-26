#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/game/number_generation.h"
#include "../../src/game/operand_group.h"

#define TEST_VALUE_COUNT (TETONOR_MAX_VALUE - TETONOR_MIN_VALUE + 1)
#define TEST_SEED 42
#define TEST_SEED_COUNT 50
#define TEST_DEFAULT_DIFFICULTY DIFFICULTY_MEDIUM
#define TEST_LARGE_DIFFICULTY 100
#define TEST_VERY_NEGATIVE_DIFFICULTY (-100)
#define TEST_LEVEL_COUNT 3
#define TEST_MIN_OPERAND 1
#define TEST_MAX_CELL_VALUE (TETONOR_MAX_VALUE * TETONOR_MAX_VALUE)

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

static int is_in_value_range(int value)
{
    return value >= TETONOR_MIN_VALUE && value <= TETONOR_MAX_VALUE;
}

/* Literal expectations on purpose: reading them back from the profile table
 * would make the tests circular. */
static const Difficulty test_levels[TEST_LEVEL_COUNT] = {
    DIFFICULTY_EASY, DIFFICULTY_MEDIUM, DIFFICULTY_HARD};
static const int test_hidden_counts[TEST_LEVEL_COUNT] = {4, 8, 12};
static const int test_max_operands[TEST_LEVEL_COUNT]  = {12, 20, 29};

static int is_derivable_from_pair(const int* numbers, int value)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        for (int j = 0; j < TETONOR_CELL_COUNT; j++)
        {
            if (i != j && (numbers[i] + numbers[j] == value ||
                           numbers[i] * numbers[j] == value))
            {
                return 1;
            }
        }
    }
    return 0;
}

static void test_generate_random_numbers_returns_non_null(void)
{
    srand(TEST_SEED);
    NumberGeneration* number_generation =
        generate_random_numbers(TEST_DEFAULT_DIFFICULTY);
    ASSERT_NOT_NULL(number_generation);
    free_number_generation(number_generation);
}

static void test_generate_random_numbers_available_values_within_range(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        srand(seed);
        NumberGeneration* number_generation =
            generate_random_numbers(TEST_DEFAULT_DIFFICULTY);
        ASSERT_NOT_NULL(number_generation);
        for (int i = 0; i < TETONOR_CELL_COUNT; i++)
        {
            int value = number_generation->available_array[i];
            if (value != TETONOR_HIDDEN_NUMBER && !is_in_value_range(value))
            {
                free_number_generation(number_generation);
                ASSERT_TRUE(is_in_value_range(value));
            }
        }
        free_number_generation(number_generation);
    }
}

static void test_generate_random_numbers_visible_values_sorted_ascending(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        srand(seed);
        NumberGeneration* number_generation =
            generate_random_numbers(TEST_DEFAULT_DIFFICULTY);
        ASSERT_NOT_NULL(number_generation);
        int previous = TETONOR_MIN_VALUE;
        for (int i = 0; i < TETONOR_CELL_COUNT; i++)
        {
            int value = number_generation->available_array[i];
            if (value == TETONOR_HIDDEN_NUMBER)
            {
                continue;
            }
            if (value < previous)
            {
                free_number_generation(number_generation);
                ASSERT_TRUE(value >= previous);
            }
            previous = value;
        }
        free_number_generation(number_generation);
    }
}

static void test_generate_random_numbers_hides_exact_count_per_level(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
        {
            srand(seed);
            NumberGeneration* number_generation =
                generate_random_numbers(test_levels[level]);
            ASSERT_NOT_NULL(number_generation);
            int hidden = count_hidden(number_generation);
            free_number_generation(number_generation);
            ASSERT_EQ_INT(hidden, test_hidden_counts[level]);
        }
    }
}

static void test_generate_random_numbers_negative_difficulty_acts_as_easy(void)
{
    const int difficulties[] = {-1, TEST_VERY_NEGATIVE_DIFFICULTY, INT_MIN};
    for (size_t d = 0; d < sizeof difficulties / sizeof difficulties[0]; d++)
    {
        srand(TEST_SEED);
        NumberGeneration* easy = generate_random_numbers(DIFFICULTY_EASY);
        srand(TEST_SEED);
        NumberGeneration* clamped = generate_random_numbers(difficulties[d]);
        bool generated            = easy != NULL && clamped != NULL;
        bool equal = generated && memcmp(easy, clamped, sizeof *easy) == 0;
        int hidden = generated ? count_hidden(clamped) : 0;
        free_number_generation(easy);
        free_number_generation(clamped);
        ASSERT_TRUE(generated);
        ASSERT_TRUE(equal);
        ASSERT_EQ_INT(hidden, test_hidden_counts[0]);
    }
}

static void test_generate_random_numbers_large_difficulty_stays_in_bounds(void)
{
    const int difficulties[] = {TEST_LARGE_DIFFICULTY, DIFFICULTY_COUNT,
                                INT_MAX};
    for (size_t d = 0; d < sizeof difficulties / sizeof difficulties[0]; d++)
    {
        srand(TEST_SEED);
        NumberGeneration* hard = generate_random_numbers(DIFFICULTY_HARD);
        srand(TEST_SEED);
        NumberGeneration* clamped = generate_random_numbers(difficulties[d]);
        bool generated            = hard != NULL && clamped != NULL;
        bool equal = generated && memcmp(hard, clamped, sizeof *hard) == 0;
        int hidden = generated ? count_hidden(clamped) : 0;
        bool values_valid = generated;
        for (int i = 0; generated && i < TETONOR_CELL_COUNT; i++)
        {
            int value = clamped->solution_array[i];
            if (value < TEST_MIN_OPERAND || value > test_max_operands[2])
            {
                values_valid = false;
            }
        }
        free_number_generation(hard);
        free_number_generation(clamped);
        ASSERT_TRUE(generated);
        ASSERT_TRUE(equal);
        ASSERT_EQ_INT(hidden, test_hidden_counts[2]);
        ASSERT_TRUE(values_valid);
    }
}

static void
test_generate_random_numbers_board_cells_derivable_from_solution_pairs(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
        {
            srand(seed);
            NumberGeneration* number_generation =
                generate_random_numbers(test_levels[level]);
            ASSERT_NOT_NULL(number_generation);
            int all_derivable = 1;
            for (int row = 0; row < TETONOR_GRID_SIZE; row++)
            {
                for (int col = 0; col < TETONOR_GRID_SIZE; col++)
                {
                    int cell = number_generation->sum_matrix[row][col];
                    if (!is_derivable_from_pair(
                            number_generation->solution_array, cell))
                    {
                        all_derivable = 0;
                    }
                }
            }
            free_number_generation(number_generation);
            ASSERT_TRUE(all_derivable);
        }
    }
}

static void test_get_difficulty_profile_pins_each_level(void)
{
    const DifficultyProfile easy   = get_difficulty_profile(DIFFICULTY_EASY);
    const DifficultyProfile medium = get_difficulty_profile(DIFFICULTY_MEDIUM);
    const DifficultyProfile hard   = get_difficulty_profile(DIFFICULTY_HARD);
    ASSERT_EQ_INT(easy.min_value, 1);
    ASSERT_EQ_INT(easy.max_value, 12);
    ASSERT_EQ_INT(easy.fully_hidden_pairs, 0);
    ASSERT_EQ_INT(easy.half_hidden_pairs, 4);
    ASSERT_EQ_INT(medium.min_value, 1);
    ASSERT_EQ_INT(medium.max_value, 20);
    ASSERT_EQ_INT(medium.fully_hidden_pairs, 0);
    ASSERT_EQ_INT(medium.half_hidden_pairs, 8);
    ASSERT_EQ_INT(hard.min_value, 1);
    ASSERT_EQ_INT(hard.max_value, 29);
    ASSERT_EQ_INT(hard.fully_hidden_pairs, 4);
    ASSERT_EQ_INT(hard.half_hidden_pairs, 4);
}

static void test_get_difficulty_profile_no_level_allows_zero(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        DifficultyProfile profile = get_difficulty_profile(test_levels[level]);
        ASSERT_EQ_INT(profile.min_value, TEST_MIN_OPERAND);
        ASSERT_TRUE(profile.min_value > TETONOR_MIN_VALUE);
    }
}

static bool profile_equals(DifficultyProfile first, DifficultyProfile second)
{
    return first.min_value == second.min_value &&
           first.max_value == second.max_value &&
           first.fully_hidden_pairs == second.fully_hidden_pairs &&
           first.half_hidden_pairs == second.half_hidden_pairs;
}

static void test_get_difficulty_profile_clamps_out_of_range(void)
{
    const int below[] = {-1, TEST_VERY_NEGATIVE_DIFFICULTY, INT_MIN};
    const int above[] = {DIFFICULTY_COUNT, TEST_LARGE_DIFFICULTY, INT_MAX};
    const DifficultyProfile easy = get_difficulty_profile(DIFFICULTY_EASY);
    const DifficultyProfile hard = get_difficulty_profile(DIFFICULTY_HARD);
    for (size_t i = 0; i < sizeof below / sizeof below[0]; i++)
    {
        ASSERT_TRUE(profile_equals(get_difficulty_profile(below[i]), easy));
        ASSERT_TRUE(profile_equals(get_difficulty_profile(above[i]), hard));
    }
}

static void test_get_difficulty_profile_ranges_inside_global_range(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        DifficultyProfile profile = get_difficulty_profile(test_levels[level]);
        ASSERT_TRUE(profile.min_value >= TETONOR_MIN_VALUE);
        ASSERT_TRUE(profile.min_value <= profile.max_value);
        ASSERT_TRUE(profile.max_value <= TETONOR_MAX_VALUE);
        ASSERT_TRUE(profile.fully_hidden_pairs >= 0);
        ASSERT_TRUE(profile.half_hidden_pairs >= 0);
        ASSERT_TRUE(profile.fully_hidden_pairs + profile.half_hidden_pairs <=
                    TETONOR_PAIR_COUNT);
    }
}

static void test_difficulty_levels_strictly_increase_hidden_and_range(void)
{
    int hidden[TEST_LEVEL_COUNT];
    int observed_max[TEST_LEVEL_COUNT];
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        observed_max[level] = 0;
        srand(TEST_SEED);
        NumberGeneration* first = generate_random_numbers(test_levels[level]);
        ASSERT_NOT_NULL(first);
        hidden[level] = count_hidden(first);
        free_number_generation(first);
        /* The largest value over all seeds: a single puzzle may stay below
         * the bound by chance, fifty of them practically cannot. */
        for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
        {
            srand(seed);
            NumberGeneration* number_generation =
                generate_random_numbers(test_levels[level]);
            ASSERT_NOT_NULL(number_generation);
            for (int i = 0; i < TETONOR_CELL_COUNT; i++)
            {
                if (number_generation->solution_array[i] > observed_max[level])
                {
                    observed_max[level] = number_generation->solution_array[i];
                }
            }
            free_number_generation(number_generation);
        }
    }
    ASSERT_TRUE(hidden[0] < hidden[1]);
    ASSERT_TRUE(hidden[1] < hidden[2]);
    ASSERT_TRUE(observed_max[0] <= test_max_operands[0]);
    ASSERT_TRUE(observed_max[1] > test_max_operands[0]);
    ASSERT_TRUE(observed_max[1] <= test_max_operands[1]);
    ASSERT_TRUE(observed_max[2] > test_max_operands[1]);
    ASSERT_TRUE(observed_max[2] <= test_max_operands[2]);
}

static void test_generate_random_numbers_levels_differ_for_same_seed(void)
{
    int hidden[TEST_LEVEL_COUNT];
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        srand(TEST_SEED);
        NumberGeneration* number_generation =
            generate_random_numbers(test_levels[level]);
        ASSERT_NOT_NULL(number_generation);
        hidden[level] = count_hidden(number_generation);
        free_number_generation(number_generation);
    }
    ASSERT_TRUE(hidden[0] != hidden[1]);
    ASSERT_TRUE(hidden[1] != hidden[2]);
    ASSERT_TRUE(hidden[0] != hidden[2]);
}

static void test_generate_random_numbers_is_deterministic_per_level(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        srand(TEST_SEED);
        NumberGeneration* first = generate_random_numbers(test_levels[level]);
        srand(TEST_SEED);
        NumberGeneration* second = generate_random_numbers(test_levels[level]);
        bool both_generated      = first != NULL && second != NULL;
        bool equal =
            both_generated && memcmp(first, second, sizeof *first) == 0;
        free_number_generation(first);
        free_number_generation(second);
        ASSERT_TRUE(both_generated);
        ASSERT_TRUE(equal);
    }
}

static void test_generate_random_numbers_values_within_level_range(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
        {
            srand(seed);
            NumberGeneration* number_generation =
                generate_random_numbers(test_levels[level]);
            ASSERT_NOT_NULL(number_generation);
            bool in_range = true;
            for (int i = 0; i < TETONOR_CELL_COUNT; i++)
            {
                int value = number_generation->solution_array[i];
                if (value < TEST_MIN_OPERAND ||
                    value > test_max_operands[level])
                {
                    in_range = false;
                }
            }
            free_number_generation(number_generation);
            ASSERT_TRUE(in_range);
        }
    }
}

static void test_generate_random_numbers_no_zero_cells(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
        {
            srand(seed);
            NumberGeneration* number_generation =
                generate_random_numbers(test_levels[level]);
            ASSERT_NOT_NULL(number_generation);
            bool cells_valid = true;
            for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
            {
                int value =
                    number_generation->sum_matrix[cell / TETONOR_GRID_SIZE]
                                                 [cell % TETONOR_GRID_SIZE];
                bool is_sum =
                    number_generation->solution_groups[cell].operation ==
                    GROUP_OPERATION_ADD;
                /* Operands are at least 1: the smallest sum is 1 + 1 and the
                 * smallest product is 1 * 1. */
                if (value < (is_sum ? 2 : 1))
                {
                    cells_valid = false;
                }
            }
            free_number_generation(number_generation);
            ASSERT_TRUE(cells_valid);
        }
    }
}

static int count_cells_with_value(const NumberGeneration* number_generation,
                                  int value)
{
    int count = 0;
    for (int row = 0; row < TETONOR_GRID_SIZE; row++)
    {
        for (int col = 0; col < TETONOR_GRID_SIZE; col++)
        {
            if (number_generation->sum_matrix[row][col] == value)
            {
                count++;
            }
        }
    }
    return count;
}

/* A hidden value is anchored by a visible one when their sum and product both
 * sit in the matrix, in two different cells (so two cells when they are equal).
 */
static bool is_anchored(const NumberGeneration* number_generation, int hidden)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        int visible = number_generation->available_array[i];
        if (visible == TETONOR_HIDDEN_NUMBER)
        {
            continue;
        }
        int sum     = visible + hidden;
        int product = visible * hidden;
        int needed  = sum == product ? 2 : 1;
        if (count_cells_with_value(number_generation, sum) >= needed &&
            count_cells_with_value(number_generation, product) >= needed)
        {
            return true;
        }
    }
    return false;
}

static void
test_generate_random_numbers_easy_and_medium_hidden_entries_anchored(void)
{
    for (int level = 0; level < 2; level++)
    {
        for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
        {
            srand(seed);
            NumberGeneration* number_generation =
                generate_random_numbers(test_levels[level]);
            ASSERT_NOT_NULL(number_generation);
            bool all_anchored = true;
            for (int i = 0; i < TETONOR_CELL_COUNT; i++)
            {
                if (number_generation->available_array[i] ==
                        TETONOR_HIDDEN_NUMBER &&
                    !is_anchored(number_generation,
                                 number_generation->solution_array[i]))
                {
                    all_anchored = false;
                }
            }
            free_number_generation(number_generation);
            ASSERT_TRUE(all_anchored);
        }
    }
}

static void test_generate_random_numbers_hidden_entries_only_hidden_marker(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
        {
            srand(seed);
            NumberGeneration* number_generation =
                generate_random_numbers(test_levels[level]);
            ASSERT_NOT_NULL(number_generation);
            bool only_marker = true;
            for (int i = 0; i < TETONOR_CELL_COUNT; i++)
            {
                int visible = number_generation->available_array[i];
                if (visible != TETONOR_HIDDEN_NUMBER &&
                    visible != number_generation->solution_array[i])
                {
                    only_marker = false;
                }
            }
            free_number_generation(number_generation);
            ASSERT_TRUE(only_marker);
        }
    }
}

static void test_generate_random_numbers_board_values_within_range(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        srand(seed);
        NumberGeneration* number_generation =
            generate_random_numbers(TEST_DEFAULT_DIFFICULTY);
        ASSERT_NOT_NULL(number_generation);
        int all_in_range = 1;
        for (int row = 0; row < TETONOR_GRID_SIZE; row++)
        {
            for (int col = 0; col < TETONOR_GRID_SIZE; col++)
            {
                int cell = number_generation->sum_matrix[row][col];
                if (cell < TETONOR_MIN_VALUE || cell > TEST_MAX_CELL_VALUE)
                {
                    all_in_range = 0;
                }
            }
        }
        free_number_generation(number_generation);
        ASSERT_TRUE(all_in_range);
    }
}

static void test_generate_random_numbers_is_deterministic_for_fixed_seed(void)
{
    srand(TEST_SEED);
    NumberGeneration* first = generate_random_numbers(TEST_DEFAULT_DIFFICULTY);
    srand(TEST_SEED);
    NumberGeneration* second = generate_random_numbers(TEST_DEFAULT_DIFFICULTY);
    int both_generated       = first != NULL && second != NULL;
    int equal = both_generated && memcmp(first, second, sizeof *first) == 0;
    free_number_generation(first);
    free_number_generation(second);
    ASSERT_TRUE(both_generated);
    ASSERT_TRUE(equal);
}

static void test_free_number_generation_accepts_null(void)
{
    free_number_generation(NULL);
    ASSERT_TRUE(1);
}

static void test_generate_random_numbers_solution_has_no_hidden_values(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        srand(seed);
        NumberGeneration* number_generation =
            generate_random_numbers(TEST_DEFAULT_DIFFICULTY);
        ASSERT_NOT_NULL(number_generation);
        for (int i = 0; i < TETONOR_CELL_COUNT; i++)
        {
            int value = number_generation->solution_array[i];
            if (value == TETONOR_HIDDEN_NUMBER)
            {
                free_number_generation(number_generation);
                ASSERT_TRUE(value != TETONOR_HIDDEN_NUMBER);
            }
        }
        free_number_generation(number_generation);
    }
}

static void test_generate_random_numbers_solution_sorted_and_in_range(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        srand(seed);
        NumberGeneration* number_generation =
            generate_random_numbers(TEST_DEFAULT_DIFFICULTY);
        ASSERT_NOT_NULL(number_generation);
        int previous = TETONOR_MIN_VALUE;
        for (int i = 0; i < TETONOR_CELL_COUNT; i++)
        {
            int value = number_generation->solution_array[i];
            if (!is_in_value_range(value) || value < previous)
            {
                free_number_generation(number_generation);
                ASSERT_TRUE(is_in_value_range(value) && value >= previous);
            }
            previous = value;
        }
        free_number_generation(number_generation);
    }
}

static void test_generate_random_numbers_visible_values_match_solution(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        srand(seed);
        NumberGeneration* number_generation =
            generate_random_numbers(TEST_DEFAULT_DIFFICULTY);
        ASSERT_NOT_NULL(number_generation);
        for (int i = 0; i < TETONOR_CELL_COUNT; i++)
        {
            int visible  = number_generation->available_array[i];
            int solution = number_generation->solution_array[i];
            if (visible != TETONOR_HIDDEN_NUMBER && visible != solution)
            {
                free_number_generation(number_generation);
                ASSERT_EQ_INT(visible, solution);
            }
        }
        free_number_generation(number_generation);
    }
}

static int cell_value(const NumberGeneration* number_generation, int cell)
{
    return number_generation
        ->sum_matrix[cell / TETONOR_GRID_SIZE][cell % TETONOR_GRID_SIZE];
}

static int apply_operation(const OperandGroup* group)
{
    return group->operation == GROUP_OPERATION_ADD
               ? group->smaller + group->greater
               : group->smaller * group->greater;
}

static bool
solution_groups_decompose_their_cells(const NumberGeneration* number_generation)
{
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        const OperandGroup* group = &number_generation->solution_groups[cell];
        bool has_operation = group->operation == GROUP_OPERATION_ADD ||
                             group->operation == GROUP_OPERATION_MULTIPLY;
        if (!has_operation || !is_in_value_range(group->smaller) ||
            !is_in_value_range(group->greater) ||
            group->smaller > group->greater ||
            apply_operation(group) != cell_value(number_generation, cell))
        {
            return false;
        }
    }
    return true;
}

static int count_operation(const NumberGeneration* number_generation,
                           GroupOperation operation)
{
    int count = 0;
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        if (number_generation->solution_groups[cell].operation == operation)
        {
            count++;
        }
    }
    return count;
}

static bool
solution_pairs_are_balanced(const NumberGeneration* number_generation)
{
    int balance[TEST_VALUE_COUNT][TEST_VALUE_COUNT] = {{0}};
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        const OperandGroup* group = &number_generation->solution_groups[cell];
        int step = group->operation == GROUP_OPERATION_ADD ? 1 : -1;
        balance[group->smaller][group->greater] += step;
    }
    for (int smaller = 0; smaller < TEST_VALUE_COUNT; smaller++)
    {
        for (int greater = 0; greater < TEST_VALUE_COUNT; greater++)
        {
            if (balance[smaller][greater] != 0)
            {
                return false;
            }
        }
    }
    return true;
}

static bool solution_operands_are_the_solution_twice(
    const NumberGeneration* number_generation)
{
    int surplus[TEST_VALUE_COUNT] = {0};
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        surplus[number_generation->solution_array[i]] += 2;
        surplus[number_generation->solution_groups[i].smaller] -= 1;
        surplus[number_generation->solution_groups[i].greater] -= 1;
    }
    for (int value = 0; value < TEST_VALUE_COUNT; value++)
    {
        if (surplus[value] != 0)
        {
            return false;
        }
    }
    return true;
}

static void test_generate_random_numbers_solution_groups_decompose_cells(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        srand(seed);
        NumberGeneration* number_generation =
            generate_random_numbers(TEST_DEFAULT_DIFFICULTY);
        ASSERT_NOT_NULL(number_generation);
        bool decomposes =
            solution_groups_decompose_their_cells(number_generation);
        free_number_generation(number_generation);
        ASSERT_TRUE(decomposes);
    }
}

static void test_generate_random_numbers_solution_groups_are_valid_groups(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        srand(seed);
        NumberGeneration* number_generation =
            generate_random_numbers(TEST_DEFAULT_DIFFICULTY);
        ASSERT_NOT_NULL(number_generation);
        bool all_valid = true;
        for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
        {
            all_valid =
                all_valid && is_operand_group_valid(
                                 &number_generation->solution_groups[cell],
                                 cell_value(number_generation, cell));
        }
        free_number_generation(number_generation);
        ASSERT_TRUE(all_valid);
    }
}

static void test_generate_random_numbers_adds_and_multiplies_each_pair(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        srand(seed);
        NumberGeneration* number_generation =
            generate_random_numbers(TEST_DEFAULT_DIFFICULTY);
        ASSERT_NOT_NULL(number_generation);
        int added = count_operation(number_generation, GROUP_OPERATION_ADD);
        int multiplied =
            count_operation(number_generation, GROUP_OPERATION_MULTIPLY);
        free_number_generation(number_generation);
        ASSERT_EQ_INT(added, TETONOR_PAIR_COUNT);
        ASSERT_EQ_INT(multiplied, TETONOR_PAIR_COUNT);
    }
}

static void test_generate_random_numbers_solution_pairs_are_balanced(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        srand(seed);
        NumberGeneration* number_generation =
            generate_random_numbers(TEST_DEFAULT_DIFFICULTY);
        ASSERT_NOT_NULL(number_generation);
        bool balanced = solution_pairs_are_balanced(number_generation);
        free_number_generation(number_generation);
        ASSERT_TRUE(balanced);
    }
}

static void test_generate_random_numbers_operands_are_solution_twice(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        srand(seed);
        NumberGeneration* number_generation =
            generate_random_numbers(TEST_DEFAULT_DIFFICULTY);
        ASSERT_NOT_NULL(number_generation);
        bool matches =
            solution_operands_are_the_solution_twice(number_generation);
        free_number_generation(number_generation);
        ASSERT_TRUE(matches);
    }
}

static void
test_generate_random_numbers_solution_groups_independent_of_mask(void)
{
    const int difficulties[] = {DIFFICULTY_EASY, DIFFICULTY_MEDIUM,
                                DIFFICULTY_HARD, TEST_LARGE_DIFFICULTY};
    for (size_t d = 0; d < sizeof difficulties / sizeof difficulties[0]; d++)
    {
        srand(TEST_SEED);
        NumberGeneration* number_generation =
            generate_random_numbers(difficulties[d]);
        ASSERT_NOT_NULL(number_generation);
        bool decomposes =
            solution_groups_decompose_their_cells(number_generation);
        free_number_generation(number_generation);
        ASSERT_TRUE(decomposes);
    }
}

static bool strip_is_sorted(const int* strip)
{
    int previous = TETONOR_MIN_VALUE;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (strip[i] == TETONOR_HIDDEN_NUMBER)
        {
            continue;
        }
        if (strip[i] < previous)
        {
            return false;
        }
        previous = strip[i];
    }
    return true;
}

static bool
groups_are_valid_operand_groups(const NumberGeneration* number_generation)
{
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        if (!is_operand_group_valid(&number_generation->solution_groups[cell],
                                    cell_value(number_generation, cell)))
        {
            return false;
        }
    }
    return true;
}

static void test_generate_random_numbers_valid_per_level(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
        {
            srand(seed);
            NumberGeneration* puzzle =
                generate_random_numbers(test_levels[level]);
            ASSERT_NOT_NULL(puzzle);
            bool sorted     = strip_is_sorted(puzzle->solution_array) &&
                              strip_is_sorted(puzzle->available_array);
            bool decomposes = solution_groups_decompose_their_cells(puzzle);
            bool valid      = groups_are_valid_operand_groups(puzzle);
            int added       = count_operation(puzzle, GROUP_OPERATION_ADD);
            int multiplied  = count_operation(puzzle, GROUP_OPERATION_MULTIPLY);
            bool balanced   = solution_pairs_are_balanced(puzzle);
            bool twice      = solution_operands_are_the_solution_twice(puzzle);
            free_number_generation(puzzle);
            ASSERT_TRUE(sorted);
            ASSERT_TRUE(decomposes);
            ASSERT_TRUE(valid);
            ASSERT_EQ_INT(added, TETONOR_PAIR_COUNT);
            ASSERT_EQ_INT(multiplied, TETONOR_PAIR_COUNT);
            ASSERT_TRUE(balanced);
            ASSERT_TRUE(twice);
        }
    }
}

#define TEST_COPY_SENTINEL 4242

static void test_alloc_number_generation_copy_equals_the_source(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        srand(TEST_SEED);
        NumberGeneration* source = generate_random_numbers(test_levels[level]);
        ASSERT_NOT_NULL(source);
        NumberGeneration* copy = alloc_number_generation_copy(source);
        ASSERT_NOT_NULL(copy);
        bool distinct = copy != source;
        bool same     = memcmp(copy, source, sizeof *source) == 0;
        free_number_generation(copy);
        free_number_generation(source);
        ASSERT_TRUE(distinct);
        ASSERT_TRUE(same);
    }
}

static void test_alloc_number_generation_copy_is_independent(void)
{
    srand(TEST_SEED);
    NumberGeneration* source = generate_random_numbers(TEST_DEFAULT_DIFFICULTY);
    ASSERT_NOT_NULL(source);
    NumberGeneration* copy = alloc_number_generation_copy(source);
    ASSERT_NOT_NULL(copy);
    NumberGeneration original = *source;
    copy->available_array[0]  = TETONOR_HIDDEN_NUMBER;
    copy->sum_matrix[0][0]    = TEST_COPY_SENTINEL;
    ASSERT_TRUE(memcmp(source, &original, sizeof original) == 0);
    source->solution_array[0] = TEST_COPY_SENTINEL;
    ASSERT_TRUE(copy->solution_array[0] != TEST_COPY_SENTINEL);
    free_number_generation(source);
    int still_readable = copy->solution_groups[0].smaller;
    free_number_generation(copy);
    ASSERT_EQ_INT(still_readable, original.solution_groups[0].smaller);
}

static void test_alloc_number_generation_copy_of_null_is_null(void)
{
    ASSERT_NULL(alloc_number_generation_copy(NULL));
}

static void test_alloc_number_generation_copy_keeps_hidden_markers(void)
{
    srand(TEST_SEED);
    NumberGeneration* source = generate_random_numbers(DIFFICULTY_HARD);
    ASSERT_NOT_NULL(source);
    NumberGeneration* copy = alloc_number_generation_copy(source);
    ASSERT_NOT_NULL(copy);
    int source_hidden = count_hidden(source);
    int copy_hidden   = count_hidden(copy);
    free_number_generation(copy);
    free_number_generation(source);
    ASSERT_EQ_INT(copy_hidden, source_hidden);
    ASSERT_EQ_INT(copy_hidden, 12);
}

int main(void)
{
    RUN_TEST(test_generate_random_numbers_returns_non_null);
    RUN_TEST(test_generate_random_numbers_available_values_within_range);
    RUN_TEST(test_generate_random_numbers_visible_values_sorted_ascending);
    RUN_TEST(test_generate_random_numbers_hides_exact_count_per_level);
    RUN_TEST(test_generate_random_numbers_negative_difficulty_acts_as_easy);
    RUN_TEST(test_generate_random_numbers_large_difficulty_stays_in_bounds);
    RUN_TEST(
        test_generate_random_numbers_board_cells_derivable_from_solution_pairs);
    RUN_TEST(test_generate_random_numbers_board_values_within_range);
    RUN_TEST(test_generate_random_numbers_is_deterministic_for_fixed_seed);
    RUN_TEST(test_free_number_generation_accepts_null);
    RUN_TEST(test_generate_random_numbers_solution_has_no_hidden_values);
    RUN_TEST(test_generate_random_numbers_solution_sorted_and_in_range);
    RUN_TEST(test_generate_random_numbers_visible_values_match_solution);
    RUN_TEST(test_generate_random_numbers_solution_groups_decompose_cells);
    RUN_TEST(test_generate_random_numbers_solution_groups_are_valid_groups);
    RUN_TEST(test_generate_random_numbers_adds_and_multiplies_each_pair);
    RUN_TEST(test_generate_random_numbers_solution_pairs_are_balanced);
    RUN_TEST(test_generate_random_numbers_operands_are_solution_twice);
    RUN_TEST(test_generate_random_numbers_solution_groups_independent_of_mask);
    RUN_TEST(test_generate_random_numbers_valid_per_level);
    RUN_TEST(test_get_difficulty_profile_pins_each_level);
    RUN_TEST(test_get_difficulty_profile_no_level_allows_zero);
    RUN_TEST(test_get_difficulty_profile_clamps_out_of_range);
    RUN_TEST(test_get_difficulty_profile_ranges_inside_global_range);
    RUN_TEST(test_difficulty_levels_strictly_increase_hidden_and_range);
    RUN_TEST(test_generate_random_numbers_levels_differ_for_same_seed);
    RUN_TEST(test_generate_random_numbers_is_deterministic_per_level);
    RUN_TEST(test_generate_random_numbers_values_within_level_range);
    RUN_TEST(test_generate_random_numbers_no_zero_cells);
    RUN_TEST(
        test_generate_random_numbers_easy_and_medium_hidden_entries_anchored);
    RUN_TEST(test_generate_random_numbers_hidden_entries_only_hidden_marker);
    RUN_TEST(test_alloc_number_generation_copy_equals_the_source);
    RUN_TEST(test_alloc_number_generation_copy_is_independent);
    RUN_TEST(test_alloc_number_generation_copy_of_null_is_null);
    RUN_TEST(test_alloc_number_generation_copy_keeps_hidden_markers);
    return test_harness_result();
}
