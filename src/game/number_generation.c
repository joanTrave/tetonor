#include <stddef.h>
#include <stdlib.h>

#include "number_generation.h"

static void populate_numbers(NumberGeneration* number_generation);
static void sort_available_array(NumberGeneration* number_generation);
static void store_solution(NumberGeneration* number_generation);
static void mask_available_array(NumberGeneration* number_generation,
                                 int difficulty);
static void fill_shuffled_positions(int positions[], size_t count);
static void swap_ints(int* a, int* b);
static int random_value(void);

NumberGeneration* generate_random_numbers(int difficulty)
{
    NumberGeneration* number_generation = calloc(1, sizeof *number_generation);
    if (number_generation == NULL)
    {
        return NULL;
    }
    populate_numbers(number_generation);
    sort_available_array(number_generation);
    store_solution(number_generation);
    mask_available_array(number_generation, difficulty);
    return number_generation;
}

void free_number_generation(NumberGeneration* number_generation)
{
    free(number_generation);
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

static void populate_numbers(NumberGeneration* number_generation)
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
        int value_1 = random_value();
        int value_2 = random_value();
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
}

static void sort_available_array(NumberGeneration* number_generation)
{
    int* values = number_generation->available_array;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        for (int j = i + 1; j < TETONOR_CELL_COUNT; j++)
        {
            if (values[i] > values[j])
            {
                swap_ints(&values[i], &values[j]);
            }
        }
    }
}

static void mask_available_array(NumberGeneration* number_generation,
                                 int difficulty)
{
    int masked_count = difficulty + TETONOR_HIDDEN_BASE_COUNT;
    for (int i = 0; i < masked_count; i++)
    {
        int position = rand() % TETONOR_CELL_COUNT;
        number_generation->available_array[position] = TETONOR_HIDDEN_NUMBER;
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

static int random_value(void)
{
    return TETONOR_MIN_VALUE + rand() % TETONOR_VALUE_COUNT;
}

static void store_solution(NumberGeneration* number_generation)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        number_generation->solution_array[i] =
            number_generation->available_array[i];
    }
}
