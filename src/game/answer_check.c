#include "answer_check.h"

#define OPERANDS_PER_STRIP_VALUE 2

static bool is_strip_sorted_in_range(const int strip[TETONOR_CELL_COUNT]);
static bool are_groups_valid(const NumberGeneration* puzzle,
                             const OperandGroup groups[TETONOR_CELL_COUNT]);
static bool operands_match_strip(const int strip[TETONOR_CELL_COUNT],
                                 const OperandGroup groups[TETONOR_CELL_COUNT]);
static bool are_pairs_balanced(const OperandGroup groups[TETONOR_CELL_COUNT]);
static int value_index(int value);

bool is_answer_correct(const NumberGeneration* puzzle,
                       const int strip[TETONOR_CELL_COUNT],
                       const OperandGroup groups[TETONOR_CELL_COUNT])
{
    return is_strip_sorted_in_range(strip) &&
           are_groups_valid(puzzle, groups) &&
           operands_match_strip(strip, groups) && are_pairs_balanced(groups);
}

static int value_index(int value)
{
    return value - TETONOR_MIN_VALUE;
}

static bool is_strip_sorted_in_range(const int strip[TETONOR_CELL_COUNT])
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (strip[i] < TETONOR_MIN_VALUE || strip[i] > TETONOR_MAX_VALUE)
        {
            return false;
        }
        if (i > 0 && strip[i] < strip[i - 1])
        {
            return false;
        }
    }
    return true;
}

static bool are_groups_valid(const NumberGeneration* puzzle,
                             const OperandGroup groups[TETONOR_CELL_COUNT])
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        int row    = i / TETONOR_GRID_SIZE;
        int col    = i % TETONOR_GRID_SIZE;
        int target = puzzle->sum_matrix[row][col];
        if (!is_operand_group_valid(&groups[i], target))
        {
            return false;
        }
    }
    return true;
}

/* Requires are_groups_valid, so every operand index is in range. */
static bool operands_match_strip(const int strip[TETONOR_CELL_COUNT],
                                 const OperandGroup groups[TETONOR_CELL_COUNT])
{
    int counts[TETONOR_VALUE_COUNT] = {0};
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        counts[value_index(strip[i])] += OPERANDS_PER_STRIP_VALUE;
        counts[value_index(groups[i].smaller)]--;
        counts[value_index(groups[i].greater)]--;
    }
    for (int v = 0; v < TETONOR_VALUE_COUNT; v++)
    {
        if (counts[v] != 0)
        {
            return false;
        }
    }
    return true;
}

/* Requires are_groups_valid. Every pair must be added as often as it is
 * multiplied. */
static bool are_pairs_balanced(const OperandGroup groups[TETONOR_CELL_COUNT])
{
    int balance[TETONOR_VALUE_COUNT][TETONOR_VALUE_COUNT] = {{0}};
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        int step = groups[i].operation == GROUP_OPERATION_ADD ? 1 : -1;
        int s    = value_index(groups[i].smaller);
        int g    = value_index(groups[i].greater);
        balance[s][g] += step;
    }
    for (int s = 0; s < TETONOR_VALUE_COUNT; s++)
    {
        for (int g = 0; g < TETONOR_VALUE_COUNT; g++)
        {
            if (balance[s][g] != 0)
            {
                return false;
            }
        }
    }
    return true;
}
