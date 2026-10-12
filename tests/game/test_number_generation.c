#include <stdlib.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/game/number_generation.h"

#define TEST_SEED 42
#define TEST_SEED_COUNT 50
#define TEST_DEFAULT_DIFFICULTY 1
#define TEST_LARGE_DIFFICULTY 100
#define TEST_VERY_NEGATIVE_DIFFICULTY (-100)
#define TEST_NO_MASK_DIFFICULTY (-TETONOR_HIDDEN_BASE_COUNT)
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

static int is_derivable_from_pair(const NumberGeneration* number_generation,
                                  int value)
{
    const int* numbers = number_generation->available_array;
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

static void
test_generate_random_numbers_hides_between_one_and_difficulty_plus_base(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        srand(seed);
        NumberGeneration* number_generation =
            generate_random_numbers(TEST_DEFAULT_DIFFICULTY);
        ASSERT_NOT_NULL(number_generation);
        int hidden = count_hidden(number_generation);
        free_number_generation(number_generation);
        ASSERT_TRUE(hidden >= 1);
        ASSERT_TRUE(hidden <=
                    TEST_DEFAULT_DIFFICULTY + TETONOR_HIDDEN_BASE_COUNT);
    }
}

static void
test_generate_random_numbers_hides_nothing_when_mask_count_not_positive(void)
{
    const int difficulties[] = {TEST_NO_MASK_DIFFICULTY,
                                TEST_VERY_NEGATIVE_DIFFICULTY};
    for (size_t d = 0; d < sizeof difficulties / sizeof difficulties[0]; d++)
    {
        srand(TEST_SEED);
        NumberGeneration* number_generation =
            generate_random_numbers(difficulties[d]);
        ASSERT_NOT_NULL(number_generation);
        int hidden = count_hidden(number_generation);
        free_number_generation(number_generation);
        ASSERT_EQ_INT(hidden, 0);
    }
}

static void test_generate_random_numbers_large_difficulty_stays_in_bounds(void)
{
    srand(TEST_SEED);
    NumberGeneration* number_generation =
        generate_random_numbers(TEST_LARGE_DIFFICULTY);
    ASSERT_NOT_NULL(number_generation);
    int hidden       = count_hidden(number_generation);
    int values_valid = 1;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        int value = number_generation->available_array[i];
        if (value != TETONOR_HIDDEN_NUMBER && !is_in_value_range(value))
        {
            values_valid = 0;
        }
    }
    free_number_generation(number_generation);
    ASSERT_TRUE(hidden >= 1);
    ASSERT_TRUE(hidden <= TETONOR_CELL_COUNT);
    ASSERT_TRUE(values_valid);
}

static void
test_generate_random_numbers_board_cells_derivable_from_available_pairs(void)
{
    for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        srand(seed);
        NumberGeneration* number_generation =
            generate_random_numbers(TEST_NO_MASK_DIFFICULTY);
        ASSERT_NOT_NULL(number_generation);
        int hidden        = count_hidden(number_generation);
        int all_derivable = 1;
        for (int row = 0; row < TETONOR_GRID_SIZE; row++)
        {
            for (int col = 0; col < TETONOR_GRID_SIZE; col++)
            {
                int cell = number_generation->sum_matrix[row][col];
                if (!is_derivable_from_pair(number_generation, cell))
                {
                    all_derivable = 0;
                }
            }
        }
        free_number_generation(number_generation);
        ASSERT_EQ_INT(hidden, 0);
        ASSERT_TRUE(all_derivable);
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

int main(void)
{
    RUN_TEST(test_generate_random_numbers_returns_non_null);
    RUN_TEST(test_generate_random_numbers_available_values_within_range);
    RUN_TEST(test_generate_random_numbers_visible_values_sorted_ascending);
    RUN_TEST(
        test_generate_random_numbers_hides_between_one_and_difficulty_plus_base);
    RUN_TEST(
        test_generate_random_numbers_hides_nothing_when_mask_count_not_positive);
    RUN_TEST(test_generate_random_numbers_large_difficulty_stays_in_bounds);
    RUN_TEST(
        test_generate_random_numbers_board_cells_derivable_from_available_pairs);
    RUN_TEST(test_generate_random_numbers_board_values_within_range);
    RUN_TEST(test_generate_random_numbers_is_deterministic_for_fixed_seed);
    RUN_TEST(test_free_number_generation_accepts_null);
    RUN_TEST(test_generate_random_numbers_solution_has_no_hidden_values);
    RUN_TEST(test_generate_random_numbers_solution_sorted_and_in_range);
    RUN_TEST(test_generate_random_numbers_visible_values_match_solution);
    return test_harness_result();
}
