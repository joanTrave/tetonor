#include "answer_check.h"

bool is_answer_correct(const NumberGeneration* puzzle,
                       const int answer[TETONOR_CELL_COUNT])
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (answer[i] != puzzle->solution_array[i])
        {
            return false;
        }
    }
    return true;
}
