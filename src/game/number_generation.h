#ifndef TETONOR_NUMBER_GENERATION_H
#define TETONOR_NUMBER_GENERATION_H

#include "operand_group.h"

#define TETONOR_GRID_SIZE 4
#define TETONOR_CELL_COUNT (TETONOR_GRID_SIZE * TETONOR_GRID_SIZE)
#define TETONOR_PAIR_COUNT (TETONOR_CELL_COUNT / 2)
#define TETONOR_MIN_VALUE 0
#define TETONOR_MAX_VALUE 29
#define TETONOR_VALUE_COUNT (TETONOR_MAX_VALUE - TETONOR_MIN_VALUE + 1)
#define TETONOR_HIDDEN_NUMBER (-1)
#define TETONOR_HIDDEN_BASE_COUNT 4

typedef struct
{
    int sum_matrix[TETONOR_GRID_SIZE][TETONOR_GRID_SIZE];
    int available_array[TETONOR_CELL_COUNT];
    /* available_array before masking: the generator's own strip. Judging does
     * not depend on it. The UI must not read it. */
    int solution_array[TETONOR_CELL_COUNT];
    /* The generator's own decomposition of every matrix cell (row-major cell
     * index). Test oracle only: judging accepts any valid solution. The UI
     * must not read it. */
    OperandGroup solution_groups[TETONOR_CELL_COUNT];
} NumberGeneration;

/* Returns NULL on allocation failure. */
NumberGeneration* generate_random_numbers(int difficulty);

/* Accepts NULL. */
void free_number_generation(NumberGeneration* number_generation);

#endif
