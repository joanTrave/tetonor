#ifndef TETONOR_ANSWER_CHECK_H
#define TETONOR_ANSWER_CHECK_H

#include <stdbool.h>

#include "number_generation.h"
#include "operand_group.h"

/* True when strip (given values plus player entries, one per available slot)
 * and groups (one per matrix cell, row-major) form a valid solution of
 * puzzle's matrix: the strip is sorted and in range, every group is valid for
 * its cell, every value is used as an operand exactly twice per occurrence in
 * the strip, and every pair of operands is used once added and once
 * multiplied. Does not read solution_array or solution_groups, so any valid
 * solution is accepted. */
bool is_answer_correct(const NumberGeneration* puzzle,
                       const int strip[TETONOR_CELL_COUNT],
                       const OperandGroup groups[TETONOR_CELL_COUNT]);

#endif
