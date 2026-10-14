#include "slot_usage.h"

#include <stdbool.h>
#include <stddef.h>

static bool is_in_range(int value)
{
    return value >= TETONOR_MIN_VALUE && value <= TETONOR_MAX_VALUE;
}

static void count_operand(int value, int uses[TETONOR_VALUE_COUNT])
{
    if (is_in_range(value))
    {
        uses[value - TETONOR_MIN_VALUE]++;
    }
}

/* Counts, per value, the number boxes that hold it in an add group and in a
 * multiply group. Group validity is not required. */
static void count_operation_uses(const OperandGroup groups[TETONOR_CELL_COUNT],
                                 int add_uses[TETONOR_VALUE_COUNT],
                                 int multiply_uses[TETONOR_VALUE_COUNT])
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        int* uses = NULL;
        switch (groups[i].operation)
        {
            case GROUP_OPERATION_NONE:
                break;
            case GROUP_OPERATION_ADD:
                uses = add_uses;
                break;
            case GROUP_OPERATION_MULTIPLY:
                uses = multiply_uses;
                break;
        }
        if (uses == NULL)
        {
            continue;
        }
        count_operand(groups[i].smaller, uses);
        count_operand(groups[i].greater, uses);
    }
}

static SlotUsage usage_of(bool holds_add, bool holds_multiply)
{
    if (holds_add && holds_multiply)
    {
        return SLOT_USAGE_TWICE;
    }
    if (holds_add || holds_multiply)
    {
        return SLOT_USAGE_ONCE;
    }
    return SLOT_USAGE_NONE;
}

void compute_slot_usages(const int strip[TETONOR_CELL_COUNT],
                         const OperandGroup groups[TETONOR_CELL_COUNT],
                         SlotUsage usages[TETONOR_CELL_COUNT])
{
    int add_uses[TETONOR_VALUE_COUNT]      = {0};
    int multiply_uses[TETONOR_VALUE_COUNT] = {0};
    int seen[TETONOR_VALUE_COUNT]          = {0};

    count_operation_uses(groups, add_uses, multiply_uses);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        usages[i] = SLOT_USAGE_NONE;
        if (!is_in_range(strip[i]))
        {
            continue;
        }
        int index = strip[i] - TETONOR_MIN_VALUE;
        int rank  = seen[index]++;
        usages[i] =
            usage_of(add_uses[index] > rank, multiply_uses[index] > rank);
    }
}
