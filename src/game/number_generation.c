#include <stdlib.h>
#include <time.h>

#include "number_generation.h"

NumberGeneration* _alloc_number_generation(void);
void _populate_numbers(NumberGeneration* number_generation);
void _sort_available_array(NumberGeneration* number_generation);
void _mask_available_array(NumberGeneration* number_generation, int difficulty);
int* _get_randomized_array_positions(void);
void _swap(int* a, int* b);

NumberGeneration* generate_random_numbers(int difficulty)
{
    NumberGeneration* number_generation = _alloc_number_generation();
    _populate_numbers(number_generation);
    _sort_available_array(number_generation);
    _mask_available_array(number_generation, difficulty);
    return number_generation;
}

void free_number_generation(NumberGeneration* number_generation)
{
    for (int i = 0; i < 4; i++)
    {
        free(number_generation->sum_matrix[i]);
    }
    free(number_generation->sum_matrix);
    free(number_generation->available_array);
    free(number_generation);
}

NumberGeneration* _alloc_number_generation(void)
{
    NumberGeneration* number_generation = malloc(sizeof(NumberGeneration));
    number_generation->sum_matrix       = malloc(sizeof(int*) * 4);

    for (int i = 0; i < 4; i++)
    {
        number_generation->sum_matrix[i] = malloc(sizeof(int) * 4);
    }

    number_generation->available_array = malloc(sizeof(int) * 16);
    return number_generation;
}

void _populate_numbers(NumberGeneration* number_generation)
{
    int* positions = _get_randomized_array_positions();

    // Each two positions in the sum_matrix_positions array are used to map a value in the sum matrix
    int* sum_matrix_positions = _get_randomized_array_positions();

    for (int i = 0; i < 8; i++)
    {
        int position_1 = positions[2 * i];
        int position_2 = positions[2 * i + 1];
        int value_1 = rand() % 30;
        int value_2 = rand() % 30;
        number_generation->available_array[position_1] = value_1;
        number_generation->available_array[position_2] = value_2;

        int sum_matrix_position_sum_x = sum_matrix_positions[2 * i] / 4;
        int sum_matrix_position_sum_y = sum_matrix_positions[2 * i] % 4;
        int sum_matrix_position_prod_x = sum_matrix_positions[2 * i + 1] / 4;
        int sum_matrix_position_prod_y = sum_matrix_positions[2 * i + 1] % 4;

        number_generation->sum_matrix[sum_matrix_position_sum_x][sum_matrix_position_sum_y] = value_1 + value_2;
        number_generation->sum_matrix[sum_matrix_position_prod_x][sum_matrix_position_prod_y] = value_1 * value_2;
    }
    free(positions);
    free(sum_matrix_positions);
}

void _sort_available_array(NumberGeneration* number_generation)
{
    for (int i = 0; i < 16; i++)
    {
        for (int j = i + 1; j < 16; j++)
        {
            if (number_generation->available_array[i] > number_generation->available_array[j])
            {
                _swap(&number_generation->available_array[i], &number_generation->available_array[j]);
            }
        }
    }
}

void _mask_available_array(NumberGeneration* number_generation, int difficulty)
{
    int num_of_masked_numbers = difficulty + 4;
    for (int i = 0; i < num_of_masked_numbers; i++)
    {
        int random_position = rand() % 16;
        number_generation->available_array[random_position] = -1;
    }
}

int* _get_randomized_array_positions(void)
{
    srand((unsigned)time(NULL));
    int *positions = malloc(sizeof(int) * 16);
    for (int i = 0; i < 16; i++)
    {
        positions[i] = i;
    }
    for (int i = 0; i < 16; i++)
    {
        size_t j = rand() % (16 - i);
        _swap(&positions[i], &positions[j]);
    }
    return positions;
}

void _swap(int* a, int* b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}
