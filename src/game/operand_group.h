#ifndef TETONOR_OPERAND_GROUP_H
#define TETONOR_OPERAND_GROUP_H

#include <stdbool.h>

#define TETONOR_EMPTY_ENTRY (-1) /* box with nothing typed */

typedef enum
{
    GROUP_OPERATION_NONE, /* operation box empty */
    GROUP_OPERATION_ADD,
    GROUP_OPERATION_MULTIPLY
} GroupOperation;

typedef enum
{
    GROUP_FIELD_SMALLER,
    GROUP_FIELD_OPERATION,
    GROUP_FIELD_GREATER
} GroupField;

#define TETONOR_GROUP_FIELD_COUNT                                              \
    (GROUP_FIELD_GREATER + 1) /* boxes per group */

/* The player's decomposition of one matrix cell. */
typedef struct
{
    int smaller; /* TETONOR_EMPTY_ENTRY when empty */
    GroupOperation operation;
    int greater; /* TETONOR_EMPTY_ENTRY when empty */
} OperandGroup;

/* Empties all three boxes. */
void clear_operand_group(OperandGroup* group);

bool is_operand_group_filled(const OperandGroup* group);

/* Filled, both numbers in the value range, smaller <= greater, and the
 * operation applied to them equals target. */
bool is_operand_group_valid(const OperandGroup* group, int target);

#endif
