#ifndef TETONOR_TESTS_GAME_FIXTURE_PUZZLE_H
#define TETONOR_TESTS_GAME_FIXTURE_PUZZLE_H

#include <stddef.h>
#include <string.h>

#include "../../src/game/number_generation.h"
#include "../../src/game/operand_group.h"

#define FIXTURE_PAIR(smaller_value, greater_value)                             \
    {(smaller_value), GROUP_OPERATION_ADD, (greater_value)},                   \
    {                                                                          \
        (smaller_value), GROUP_OPERATION_MULTIPLY, (greater_value)             \
    }

/* Fixture F-A: eight pairs (1,2) (3,4) (5,6) (7,8) (9,10) (11,12) (13,14)
 * (0,29), one pair per two cells: the even cell holds the sum, the odd cell the
 * product. The strip is sorted; index 15 holds the only two-digit value that is
 * also the maximum. */
static const int fixture_strip[TETONOR_CELL_COUNT] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 29};
static const int fixture_sums[TETONOR_CELL_COUNT] = {
    3, 2, 7, 12, 11, 30, 15, 56, 19, 90, 23, 132, 27, 182, 29, 0};
static const OperandGroup fixture_groups[TETONOR_CELL_COUNT] = {
    FIXTURE_PAIR(1, 2),   FIXTURE_PAIR(3, 4),  FIXTURE_PAIR(5, 6),
    FIXTURE_PAIR(7, 8),   FIXTURE_PAIR(9, 10), FIXTURE_PAIR(11, 12),
    FIXTURE_PAIR(13, 14), FIXTURE_PAIR(0, 29)};

/* Builds the F-A puzzle by hand. hidden lists the available-row indexes that
 * the player must fill in. */
static void build_fixture_puzzle(NumberGeneration* puzzle, const int* hidden,
                                 size_t hidden_count)
{
    memset(puzzle, 0, sizeof *puzzle);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        puzzle->sum_matrix[i / TETONOR_GRID_SIZE][i % TETONOR_GRID_SIZE] =
            fixture_sums[i];
        puzzle->available_array[i] = fixture_strip[i];
        puzzle->solution_array[i]  = fixture_strip[i];
        puzzle->solution_groups[i] = fixture_groups[i];
    }
    for (size_t i = 0; i < hidden_count; i++)
    {
        puzzle->available_array[hidden[i]] = TETONOR_HIDDEN_NUMBER;
    }
}

#endif
