#ifndef TETONOR_ANSWER_CHECK_H
#define TETONOR_ANSWER_CHECK_H

#include <stdbool.h>

#include "number_generation.h"

/* True when answer (given values plus player entries, one per available slot)
 * solves puzzle under the game's judging rule. */
bool is_answer_correct(const NumberGeneration* puzzle,
                       const int answer[TETONOR_CELL_COUNT]);

#endif
