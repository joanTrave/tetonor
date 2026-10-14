#include "operand_group.h"
#include "number_generation.h"

static bool is_in_value_range(int value);
static bool operation_result(const OperandGroup* group, int* result);

void clear_operand_group(OperandGroup* group)
{
    group->smaller   = TETONOR_EMPTY_ENTRY;
    group->operation = GROUP_OPERATION_NONE;
    group->greater   = TETONOR_EMPTY_ENTRY;
}

bool is_operand_group_filled(const OperandGroup* group)
{
    return group->smaller != TETONOR_EMPTY_ENTRY &&
           group->operation != GROUP_OPERATION_NONE &&
           group->greater != TETONOR_EMPTY_ENTRY;
}

bool is_operand_group_valid(const OperandGroup* group, int target)
{
    if (!is_operand_group_filled(group) || !is_in_value_range(group->smaller) ||
        !is_in_value_range(group->greater) || group->smaller > group->greater)
    {
        return false;
    }
    int result = 0;
    return operation_result(group, &result) && result == target;
}

static bool is_in_value_range(int value)
{
    return value >= TETONOR_MIN_VALUE && value <= TETONOR_MAX_VALUE;
}

/* Largest product is 29 x 29, which fits int. Returns false for NONE. */
static bool operation_result(const OperandGroup* group, int* result)
{
    switch (group->operation)
    {
        case GROUP_OPERATION_ADD:
            *result = group->smaller + group->greater;
            return true;
        case GROUP_OPERATION_MULTIPLY:
            *result = group->smaller * group->greater;
            return true;
        case GROUP_OPERATION_NONE:
            break;
    }
    return false;
}
