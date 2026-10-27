#ifndef TETONOR_CONFLICT_CHECK_H
#define TETONOR_CONFLICT_CHECK_H

#include <stdbool.h>

#include "number_generation.h"
#include "operand_group.h"

/* Largest BoardConflicts.count: every strip slot and every group. */
#define TETONOR_CONFLICT_COUNT_MAX (2 * TETONOR_CELL_COUNT)

/* Boxes whose entry certainly breaks a rule of is_answer_correct: no valid
 * solution keeps all the boxes of one finding as they are. Empty boxes and
 * given strip numbers are never flagged. */
typedef struct
{
    bool slots[TETONOR_CELL_COUNT]; /* typed strip slot out of order/range */
    bool fields[TETONOR_CELL_COUNT]
               [TETONOR_GROUP_FIELD_COUNT]; /* [cell][GroupField] */
    int count; /* flagged slots plus groups with a flagged box */
} BoardConflicts;

/* Writes the conflicts of the current entries: strip as build_strip makes it
 * (TETONOR_EMPTY_ENTRY when empty), groups one per matrix cell (row-major).
 * A slot is editable when puzzle->available_array holds TETONOR_HIDDEN_NUMBER.
 * Pure: reads only its inputs, writes only *conflicts. */
void find_board_conflicts(const NumberGeneration* puzzle,
                          const int strip[TETONOR_CELL_COUNT],
                          const OperandGroup groups[TETONOR_CELL_COUNT],
                          BoardConflicts* conflicts);

/* True when any box of the group of cell is flagged; false for a cell out of
 * 0..TETONOR_CELL_COUNT-1. */
bool group_has_conflict(const BoardConflicts* conflicts, int cell);

#endif
