#ifndef TETONOR_SLOT_USAGE_H
#define TETONOR_SLOT_USAGE_H

#include "number_generation.h"
#include "operand_group.h"

/* How many uses a strip slot holds: at most one add use and one multiply use
 * per slot. */
typedef enum
{
    SLOT_USAGE_NONE, /* no use, or the slot is empty */
    SLOT_USAGE_ONCE, /* exactly one of an add use and a multiply use */
    SLOT_USAGE_TWICE /* an add use and a multiply use */
} SlotUsage;

/* Writes the usage of every strip slot. strip holds one value per available
 * slot (TETONOR_EMPTY_ENTRY when empty), groups one group per matrix cell.
 * Pure: reads nothing else. */
void compute_slot_usages(const int strip[TETONOR_CELL_COUNT],
                         const OperandGroup groups[TETONOR_CELL_COUNT],
                         SlotUsage usages[TETONOR_CELL_COUNT]);

#endif
