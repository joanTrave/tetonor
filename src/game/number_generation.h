#ifndef TETONOR_NUMBER_GENERATION_H
#define TETONOR_NUMBER_GENERATION_H

typedef struct
{
    int** sum_matrix;
    int* available_array;
} NumberGeneration;

NumberGeneration* generate_random_numbers(int difficulty);
void free_number_generation(NumberGeneration* number_generation);

#endif
