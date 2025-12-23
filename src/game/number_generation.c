#include <stdlib.h>

#include "number_generation.h"

NumberGeneration *_alloc_number_generation(void);
void _populate_numbers(NumberGeneration *number_generation, int difficulty);

NumberGeneration *generate_random_numbers(int difficulty) {
    NumberGeneration *number_generation = _alloc_number_generation();
    _populate_numbers(number_generation, difficulty);
    return number_generation;
}

void free_number_generation(NumberGeneration *number_generation) {
    for (int i = 0; i < 4; i++) {
        free(number_generation->sum_matrix[i]);
    }
    free(number_generation->sum_matrix);
    free(number_generation->available_array);
    free(number_generation);
}

NumberGeneration *_alloc_number_generation(void) {
    NumberGeneration *number_generation = malloc(sizeof(NumberGeneration));
    number_generation->sum_matrix = malloc(sizeof(int *) * 4);
    for (int i = 0; i < 4; i++) {
        number_generation->sum_matrix[i] = malloc(sizeof(int) * 4);
    }
    number_generation->available_array = malloc(sizeof(int) * 16);
    return number_generation;
}

void _populate_numbers(NumberGeneration *number_generation, int difficulty) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            number_generation->sum_matrix[i][j] = rand() % 350 * difficulty;
        }
    }

    for (int i = 0; i < 16; i++) {
        number_generation->available_array[i] = rand() % 30;
    }
}
