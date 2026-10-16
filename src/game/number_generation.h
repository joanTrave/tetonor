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

/* Values of the difficulty passed to generate_random_numbers. */
typedef enum
{
    DIFFICULTY_EASY,
    DIFFICULTY_MEDIUM,
    DIFFICULTY_HARD,
    DIFFICULTY_COUNT /* number of levels, not a level */
} Difficulty;

/* How a level shapes a puzzle. Easy and Medium hide at most one value per
 * pair, so every hidden number has a visible partner; Hard hides both values
 * of some pairs. Ranges stay inside TETONOR_MIN_VALUE..TETONOR_MAX_VALUE.
 * These are structural guarantees, not a measured human difficulty. */
typedef struct
{
    int min_value;          /* smallest value a pair may hold */
    int max_value;          /* largest value a pair may hold */
    int fully_hidden_pairs; /* pairs with both values hidden in the strip */
    int half_hidden_pairs;  /* pairs with exactly one value hidden */
} DifficultyProfile;

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

/* Profile of a level. Values below DIFFICULTY_EASY act as Easy, values above
 * DIFFICULTY_HARD act as Hard. */
DifficultyProfile get_difficulty_profile(int difficulty);

/* difficulty is a Difficulty, and other values are clamped as in
 * get_difficulty_profile. Returns NULL on allocation failure. */
NumberGeneration* generate_random_numbers(int difficulty);

/* Accepts NULL. */
void free_number_generation(NumberGeneration* number_generation);

#endif
