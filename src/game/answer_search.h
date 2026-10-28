#ifndef TETONOR_ANSWER_SEARCH_H
#define TETONOR_ANSWER_SEARCH_H

#include <stdbool.h>

#include "number_generation.h"
#include "operand_group.h"

/* Most search nodes find_answer_completion visits before it gives up. */
#define TETONOR_ANSWER_SEARCH_NODE_LIMIT 200000L

/* Looks for an answer that is_answer_correct accepts and that keeps every
 * entry: strip as build_board_strip makes it (TETONOR_EMPTY_ENTRY = free
 * editable slot), groups one per matrix cell (row-major) with any subset of
 * their boxes filled. On success writes the full strip and groups and returns
 * true. Returns false, leaving the outputs untouched, when no such answer
 * exists, when an entry is out of range or the strip is not sorted, or when the
 * search visits more than TETONOR_ANSWER_SEARCH_NODE_LIMIT nodes.
 * Deterministic; pure: reads only its inputs, never solution_array or
 * solution_groups. */
bool find_answer_completion(const NumberGeneration* puzzle,
                            const int strip[TETONOR_CELL_COUNT],
                            const OperandGroup groups[TETONOR_CELL_COUNT],
                            int completed_strip[TETONOR_CELL_COUNT],
                            OperandGroup completed_groups[TETONOR_CELL_COUNT]);

#endif
