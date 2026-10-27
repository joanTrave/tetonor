#include "conflict_check.h"

#include <string.h>

#define OPERANDS_PER_STRIP_VALUE 2
#define NUMBER_FIELD_COUNT 2

typedef struct
{
    int add_uses[TETONOR_VALUE_COUNT];
    int multiply_uses[TETONOR_VALUE_COUNT];
    int all_uses[TETONOR_VALUE_COUNT];
} ValueUses;

typedef struct
{
    int add[TETONOR_VALUE_COUNT][TETONOR_VALUE_COUNT];
    int multiply[TETONOR_VALUE_COUNT][TETONOR_VALUE_COUNT];
} PairUses;

/* The two boxes of a group that hold a number. */
static const GroupField number_fields[NUMBER_FIELD_COUNT] = {
    GROUP_FIELD_SMALLER, GROUP_FIELD_GREATER};

static bool is_in_range(int value);
static bool is_editable(const NumberGeneration* puzzle, int slot);
static int target_of(const NumberGeneration* puzzle, int cell);
static int number_of(const OperandGroup* group, GroupField field);
static void flag_field(BoardConflicts* conflicts, int cell, GroupField field);
static void flag_group(BoardConflicts* conflicts, int cell);
static void find_group_conflicts(const NumberGeneration* puzzle,
                                 const OperandGroup groups[TETONOR_CELL_COUNT],
                                 BoardConflicts* conflicts);
static void find_strip_conflicts(const NumberGeneration* puzzle,
                                 const int strip[TETONOR_CELL_COUNT],
                                 BoardConflicts* conflicts);
static bool is_strip_slot_out_of_order(const int strip[TETONOR_CELL_COUNT],
                                       int slot);
static void compute_capacities(const NumberGeneration* puzzle,
                               int capacity[TETONOR_VALUE_COUNT]);
static int lower_bound_of(const NumberGeneration* puzzle, int slot);
static int upper_bound_of(const NumberGeneration* puzzle, int slot);
static void count_value_uses(const OperandGroup groups[TETONOR_CELL_COUNT],
                             ValueUses* uses);
static void find_overused_values(const OperandGroup groups[TETONOR_CELL_COUNT],
                                 const int capacity[TETONOR_VALUE_COUNT],
                                 const ValueUses* uses,
                                 BoardConflicts* conflicts);
static void flag_overused_number(BoardConflicts* conflicts, int cell,
                                 GroupField field, const OperandGroup* group,
                                 const int capacity[TETONOR_VALUE_COUNT],
                                 const ValueUses* uses);
static bool is_operation_overused(GroupOperation operation, int value,
                                  const int capacity[TETONOR_VALUE_COUNT],
                                  const ValueUses* uses);
static bool is_pairable(const OperandGroup* group);
static void count_pair_uses(const OperandGroup groups[TETONOR_CELL_COUNT],
                            PairUses* pairs);
static int cells_with_target(const NumberGeneration* puzzle, int target);
static bool is_pair_overused(const NumberGeneration* puzzle,
                             const OperandGroup* group, const PairUses* pairs);
static void find_overused_pairs(const NumberGeneration* puzzle,
                                const OperandGroup groups[TETONOR_CELL_COUNT],
                                BoardConflicts* conflicts);
static void count_flagged_items(BoardConflicts* conflicts);
static int value_index(int value);

void find_board_conflicts(const NumberGeneration* puzzle,
                          const int strip[TETONOR_CELL_COUNT],
                          const OperandGroup groups[TETONOR_CELL_COUNT],
                          BoardConflicts* conflicts)
{
    int capacity[TETONOR_VALUE_COUNT];
    ValueUses uses;

    memset(conflicts, 0, sizeof *conflicts);
    find_group_conflicts(puzzle, groups, conflicts);
    find_strip_conflicts(puzzle, strip, conflicts);
    compute_capacities(puzzle, capacity);
    count_value_uses(groups, &uses);
    find_overused_values(groups, capacity, &uses, conflicts);
    find_overused_pairs(puzzle, groups, conflicts);
    count_flagged_items(conflicts);
}

bool group_has_conflict(const BoardConflicts* conflicts, int cell)
{
    if (cell < 0 || cell >= TETONOR_CELL_COUNT)
    {
        return false;
    }
    for (int field = 0; field < TETONOR_GROUP_FIELD_COUNT; field++)
    {
        if (conflicts->fields[cell][field])
        {
            return true;
        }
    }
    return false;
}

static int value_index(int value)
{
    return value - TETONOR_MIN_VALUE;
}

static bool is_in_range(int value)
{
    return value >= TETONOR_MIN_VALUE && value <= TETONOR_MAX_VALUE;
}

static bool is_editable(const NumberGeneration* puzzle, int slot)
{
    return puzzle->available_array[slot] == TETONOR_HIDDEN_NUMBER;
}

static int target_of(const NumberGeneration* puzzle, int cell)
{
    return puzzle
        ->sum_matrix[cell / TETONOR_GRID_SIZE][cell % TETONOR_GRID_SIZE];
}

/* field must be GROUP_FIELD_SMALLER or GROUP_FIELD_GREATER. */
static int number_of(const OperandGroup* group, GroupField field)
{
    return field == GROUP_FIELD_SMALLER ? group->smaller : group->greater;
}

static void flag_field(BoardConflicts* conflicts, int cell, GroupField field)
{
    conflicts->fields[cell][field] = true;
}

static void flag_group(BoardConflicts* conflicts, int cell)
{
    flag_field(conflicts, cell, GROUP_FIELD_SMALLER);
    flag_field(conflicts, cell, GROUP_FIELD_OPERATION);
    flag_field(conflicts, cell, GROUP_FIELD_GREATER);
}

/* K-1, K-2, K-3: restate is_operand_group_valid on what is typed so far. */
static void find_group_conflicts(const NumberGeneration* puzzle,
                                 const OperandGroup groups[TETONOR_CELL_COUNT],
                                 BoardConflicts* conflicts)
{
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        const OperandGroup* group = &groups[cell];
        for (int i = 0; i < NUMBER_FIELD_COUNT; i++)
        {
            int number = number_of(group, number_fields[i]);
            if (number != TETONOR_EMPTY_ENTRY && !is_in_range(number))
            {
                flag_field(conflicts, cell, number_fields[i]);
            }
        }
        if (group->smaller != TETONOR_EMPTY_ENTRY &&
            group->greater != TETONOR_EMPTY_ENTRY &&
            group->smaller > group->greater)
        {
            flag_field(conflicts, cell, GROUP_FIELD_SMALLER);
            flag_field(conflicts, cell, GROUP_FIELD_GREATER);
        }
        if (is_operand_group_filled(group) &&
            !is_operand_group_valid(group, target_of(puzzle, cell)))
        {
            flag_group(conflicts, cell);
        }
    }
}

/* K-4, K-5: restate is_strip_sorted_in_range for the typed slots. */
static void find_strip_conflicts(const NumberGeneration* puzzle,
                                 const int strip[TETONOR_CELL_COUNT],
                                 BoardConflicts* conflicts)
{
    for (int slot = 0; slot < TETONOR_CELL_COUNT; slot++)
    {
        if (!is_editable(puzzle, slot) || strip[slot] == TETONOR_EMPTY_ENTRY)
        {
            continue;
        }
        if (!is_in_range(strip[slot]) ||
            is_strip_slot_out_of_order(strip, slot))
        {
            conflicts->slots[slot] = true;
        }
    }
}

/* True when an in-range entry elsewhere in the strip sits on the wrong side of
 * the in-range value at slot. */
static bool is_strip_slot_out_of_order(const int strip[TETONOR_CELL_COUNT],
                                       int slot)
{
    for (int other = 0; other < TETONOR_CELL_COUNT; other++)
    {
        if (other == slot || strip[other] == TETONOR_EMPTY_ENTRY ||
            !is_in_range(strip[other]))
        {
            continue;
        }
        if ((other < slot && strip[other] > strip[slot]) ||
            (other > slot && strip[other] < strip[slot]))
        {
            return true;
        }
    }
    return false;
}

/* K-6: how many times each value can at most appear in the strip. A given
 * slot holds its own value; an editable slot may still hold any value between
 * its given neighbours, whatever is typed in it now. */
static void compute_capacities(const NumberGeneration* puzzle,
                               int capacity[TETONOR_VALUE_COUNT])
{
    memset(capacity, 0, sizeof(int) * TETONOR_VALUE_COUNT);
    for (int slot = 0; slot < TETONOR_CELL_COUNT; slot++)
    {
        if (!is_editable(puzzle, slot))
        {
            if (is_in_range(puzzle->available_array[slot]))
            {
                capacity[value_index(puzzle->available_array[slot])]++;
            }
            continue;
        }
        int upper = upper_bound_of(puzzle, slot);
        for (int value = lower_bound_of(puzzle, slot); value <= upper; value++)
        {
            capacity[value_index(value)]++;
        }
    }
}

/* Largest given value before slot, TETONOR_MIN_VALUE when there is none. */
static int lower_bound_of(const NumberGeneration* puzzle, int slot)
{
    for (int other = slot - 1; other >= 0; other--)
    {
        if (!is_editable(puzzle, other))
        {
            return puzzle->available_array[other];
        }
    }
    return TETONOR_MIN_VALUE;
}

/* Smallest given value after slot, TETONOR_MAX_VALUE when there is none. */
static int upper_bound_of(const NumberGeneration* puzzle, int slot)
{
    for (int other = slot + 1; other < TETONOR_CELL_COUNT; other++)
    {
        if (!is_editable(puzzle, other))
        {
            return puzzle->available_array[other];
        }
    }
    return TETONOR_MAX_VALUE;
}

static void count_value_uses(const OperandGroup groups[TETONOR_CELL_COUNT],
                             ValueUses* uses)
{
    memset(uses, 0, sizeof *uses);
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        const OperandGroup* group = &groups[cell];
        for (int i = 0; i < NUMBER_FIELD_COUNT; i++)
        {
            int number = number_of(group, number_fields[i]);
            if (!is_in_range(number))
            {
                continue;
            }
            int index = value_index(number);
            uses->all_uses[index]++;
            if (group->operation == GROUP_OPERATION_ADD)
            {
                uses->add_uses[index]++;
            }
            else if (group->operation == GROUP_OPERATION_MULTIPLY)
            {
                uses->multiply_uses[index]++;
            }
        }
    }
}

/* K-7: a value used more often than the strip can hold it. */
static void find_overused_values(const OperandGroup groups[TETONOR_CELL_COUNT],
                                 const int capacity[TETONOR_VALUE_COUNT],
                                 const ValueUses* uses,
                                 BoardConflicts* conflicts)
{
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        for (int i = 0; i < NUMBER_FIELD_COUNT; i++)
        {
            flag_overused_number(conflicts, cell, number_fields[i],
                                 &groups[cell], capacity, uses);
        }
    }
}

static void flag_overused_number(BoardConflicts* conflicts, int cell,
                                 GroupField field, const OperandGroup* group,
                                 const int capacity[TETONOR_VALUE_COUNT],
                                 const ValueUses* uses)
{
    int value = number_of(group, field);
    if (!is_in_range(value))
    {
        return;
    }
    int index = value_index(value);
    if (is_operation_overused(group->operation, value, capacity, uses))
    {
        flag_field(conflicts, cell, field);
        flag_field(conflicts, cell, GROUP_FIELD_OPERATION);
    }
    if (uses->all_uses[index] > OPERANDS_PER_STRIP_VALUE * capacity[index])
    {
        flag_field(conflicts, cell, field);
    }
}

/* True when value is used with operation more often than the strip can hold
 * it. An empty operation never is. */
static bool is_operation_overused(GroupOperation operation, int value,
                                  const int capacity[TETONOR_VALUE_COUNT],
                                  const ValueUses* uses)
{
    int index = value_index(value);
    switch (operation)
    {
        case GROUP_OPERATION_ADD:
            return uses->add_uses[index] > capacity[index];
        case GROUP_OPERATION_MULTIPLY:
            return uses->multiply_uses[index] > capacity[index];
        case GROUP_OPERATION_NONE:
            break;
    }
    return false;
}

/* A filled group over in-range, ordered numbers. */
static bool is_pairable(const OperandGroup* group)
{
    return is_operand_group_filled(group) && is_in_range(group->smaller) &&
           is_in_range(group->greater) && group->smaller <= group->greater;
}

static void count_pair_uses(const OperandGroup groups[TETONOR_CELL_COUNT],
                            PairUses* pairs)
{
    memset(pairs, 0, sizeof *pairs);
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        const OperandGroup* group = &groups[cell];
        if (!is_pairable(group))
        {
            continue;
        }
        int s = value_index(group->smaller);
        int g = value_index(group->greater);
        if (group->operation == GROUP_OPERATION_ADD)
        {
            pairs->add[s][g]++;
        }
        else
        {
            pairs->multiply[s][g]++;
        }
    }
}

static int cells_with_target(const NumberGeneration* puzzle, int target)
{
    int cells = 0;
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        cells += target_of(puzzle, cell) == target ? 1 : 0;
    }
    return cells;
}

/* Requires is_pairable(group). */
static bool is_pair_overused(const NumberGeneration* puzzle,
                             const OperandGroup* group, const PairUses* pairs)
{
    int s = value_index(group->smaller);
    int g = value_index(group->greater);
    if (group->operation == GROUP_OPERATION_ADD)
    {
        return pairs->add[s][g] >
               cells_with_target(puzzle, group->smaller * group->greater);
    }
    return pairs->multiply[s][g] >
           cells_with_target(puzzle, group->smaller + group->greater);
}

/* K-8: balance forces every pair added in one cell to be multiplied in another
 * cell holding its product (and the reverse), so a pair cannot be used with
 * one operation in more cells than the matrix has for the other. */
static void find_overused_pairs(const NumberGeneration* puzzle,
                                const OperandGroup groups[TETONOR_CELL_COUNT],
                                BoardConflicts* conflicts)
{
    PairUses pairs;
    count_pair_uses(groups, &pairs);
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        if (is_pairable(&groups[cell]) &&
            is_pair_overused(puzzle, &groups[cell], &pairs))
        {
            flag_group(conflicts, cell);
        }
    }
}

/* K-9: the items the player sees marked. */
static void count_flagged_items(BoardConflicts* conflicts)
{
    conflicts->count = 0;
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        conflicts->count += conflicts->slots[cell] ? 1 : 0;
        conflicts->count += group_has_conflict(conflicts, cell) ? 1 : 0;
    }
}
