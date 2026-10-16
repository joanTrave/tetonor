#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "number_generation.h"

/* One row per Difficulty. Every range starts at 1, so no level generates 0.
 * The levels are ordered by structure (hidden entries, both-hidden pairs,
 * operand range), not by a measured human difficulty. */
static const DifficultyProfile difficulty_profiles[DIFFICULTY_COUNT] = {
    /* 4 hidden entries, each with a visible partner; small numbers. */
    [DIFFICULTY_EASY] = {1, 12, 0, 4},
    /* 8 hidden entries, each with a visible partner. */
    [DIFFICULTY_MEDIUM] = {1, 20, 0, 8},
    /* 12 hidden entries: 4 pairs have both values hidden; large numbers. */
    [DIFFICULTY_HARD] = {1, 29, 4, 4},
};

static int clamp_difficulty(int difficulty);
static void populate_numbers(NumberGeneration* number_generation,
                             const DifficultyProfile* profile, bool hidden[]);
static void mark_hidden_members(bool hidden[], const int positions[],
                                const DifficultyProfile* profile);
static void sort_available_array(NumberGeneration* number_generation,
                                 bool hidden[]);
static void store_solution(NumberGeneration* number_generation);
static void mask_available_array(NumberGeneration* number_generation,
                                 const bool hidden[]);
static void fill_shuffled_positions(int positions[], size_t count);
static void swap_ints(int* a, int* b);
static int random_value(const DifficultyProfile* profile);

DifficultyProfile get_difficulty_profile(int difficulty)
{
    return difficulty_profiles[clamp_difficulty(difficulty)];
}

NumberGeneration* generate_random_numbers(int difficulty)
{
    DifficultyProfile profile           = get_difficulty_profile(difficulty);
    NumberGeneration* number_generation = calloc(1, sizeof *number_generation);
    if (number_generation == NULL)
    {
        return NULL;
    }
    bool hidden[TETONOR_CELL_COUNT] = {false};
    populate_numbers(number_generation, &profile, hidden);
    sort_available_array(number_generation, hidden);
    store_solution(number_generation);
    mask_available_array(number_generation, hidden);
    return number_generation;
}

void free_number_generation(NumberGeneration* number_generation)
{
    free(number_generation);
}

static int clamp_difficulty(int difficulty)
{
    if (difficulty < DIFFICULTY_EASY)
    {
        return DIFFICULTY_EASY;
    }
    if (difficulty > DIFFICULTY_HARD)
    {
        return DIFFICULTY_HARD;
    }
    return difficulty;
}

static void set_available_value(NumberGeneration* number_generation,
                                int position, int value)
{
    number_generation->available_array[position] = value;
}

static void set_matrix_cell(NumberGeneration* number_generation, int cell,
                            int value)
{
    int row                                 = cell / TETONOR_GRID_SIZE;
    int col                                 = cell % TETONOR_GRID_SIZE;
    number_generation->sum_matrix[row][col] = value;
}

static void set_solution_group(NumberGeneration* number_generation, int cell,
                               int value_1, int value_2,
                               GroupOperation operation)
{
    OperandGroup* group = &number_generation->solution_groups[cell];
    group->smaller      = value_1 < value_2 ? value_1 : value_2;
    group->operation    = operation;
    group->greater      = value_1 < value_2 ? value_2 : value_1;
}

static void populate_numbers(NumberGeneration* number_generation,
                             const DifficultyProfile* profile, bool hidden[])
{
    int positions[TETONOR_CELL_COUNT];
    int sum_matrix_positions[TETONOR_CELL_COUNT];
    fill_shuffled_positions(positions, TETONOR_CELL_COUNT);
    fill_shuffled_positions(sum_matrix_positions, TETONOR_CELL_COUNT);

    /* Each pair of shuffled positions receives one pair of values: the
     * available array stores the values, the matrix stores their sum and
     * product. */
    for (int i = 0; i < TETONOR_PAIR_COUNT; i++)
    {
        int value_1 = random_value(profile);
        int value_2 = random_value(profile);
        set_available_value(number_generation, positions[2 * i], value_1);
        set_available_value(number_generation, positions[2 * i + 1], value_2);
        int sum_cell     = sum_matrix_positions[2 * i];
        int product_cell = sum_matrix_positions[2 * i + 1];
        set_matrix_cell(number_generation, sum_cell, value_1 + value_2);
        set_matrix_cell(number_generation, product_cell, value_1 * value_2);
        set_solution_group(number_generation, sum_cell, value_1, value_2,
                           GROUP_OPERATION_ADD);
        set_solution_group(number_generation, product_cell, value_1, value_2,
                           GROUP_OPERATION_MULTIPLY);
    }
    mark_hidden_members(hidden, positions, profile);
}

/* Pair values are independent and the positions are a uniform shuffle, so
 * taking the first pairs and their first member is an unbiased choice that
 * needs no extra rand() call. */
static void mark_hidden_members(bool hidden[], const int positions[],
                                const DifficultyProfile* profile)
{
    int hidden_pairs = profile->fully_hidden_pairs + profile->half_hidden_pairs;
    for (int i = 0; i < hidden_pairs; i++)
    {
        hidden[positions[2 * i]] = true;
        if (i < profile->fully_hidden_pairs)
        {
            hidden[positions[2 * i + 1]] = true;
        }
    }
}

/* Sorts the strip ascending; a hidden flag stays with its value. */
static void sort_available_array(NumberGeneration* number_generation,
                                 bool hidden[])
{
    int* values = number_generation->available_array;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        for (int j = i + 1; j < TETONOR_CELL_COUNT; j++)
        {
            if (values[i] > values[j])
            {
                swap_ints(&values[i], &values[j]);
                bool flag = hidden[i];
                hidden[i] = hidden[j];
                hidden[j] = flag;
            }
        }
    }
}

static void mask_available_array(NumberGeneration* number_generation,
                                 const bool hidden[])
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (hidden[i])
        {
            number_generation->available_array[i] = TETONOR_HIDDEN_NUMBER;
        }
    }
}

static void fill_shuffled_positions(int positions[], size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        positions[i] = (int)i;
    }
    for (size_t i = 0; i + 1 < count; i++)
    {
        size_t j = i + (size_t)rand() % (count - i);
        swap_ints(&positions[i], &positions[j]);
    }
}

static void swap_ints(int* a, int* b)
{
    int temp = *a;
    *a       = *b;
    *b       = temp;
}

static int random_value(const DifficultyProfile* profile)
{
    int value_count = profile->max_value - profile->min_value + 1;
    return profile->min_value + rand() % value_count;
}

static void store_solution(NumberGeneration* number_generation)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        number_generation->solution_array[i] =
            number_generation->available_array[i];
    }
}
