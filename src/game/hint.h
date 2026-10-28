#ifndef TETONOR_HINT_H
#define TETONOR_HINT_H

#include <stdbool.h>

#include "board.h"
#include "operand_group.h"

typedef enum
{
    HINT_STATUS_SUGGESTION,   /* target/index/value/group hold the suggestion */
    HINT_STATUS_SOLVED,       /* the board is locked */
    HINT_STATUS_CONFLICTS,    /* a marked conflict must be fixed first */
    HINT_STATUS_BOARD_FULL,   /* nothing left to fill */
    HINT_STATUS_NO_SUGGESTION /* no valid answer keeps the current entries */
} HintStatus;

typedef enum
{
    HINT_TARGET_SLOT, /* index is a strip slot; value is its number */
    HINT_TARGET_GROUP /* index is a matrix cell; group is its decomposition */
} HintTarget;

/* A fixed-size value: nothing to release. Fields that do not apply to the
 * reply are zero. */
typedef struct
{
    HintStatus status;
    HintTarget target;  /* meaningful only for HINT_STATUS_SUGGESTION */
    int index;          /* slot index or cell index (row-major) */
    int value;          /* HINT_TARGET_SLOT only */
    OperandGroup group; /* HINT_TARGET_GROUP only; always filled */
} BoardHint;

/* The hint for the current board. Status order: solved, conflicts, board full,
 * no suggestion. The target of a suggestion is, in order: the focused empty
 * slot; the focused group when not filled; the first group not filled (cell
 * order); the first empty editable slot (index order). Only when it returns
 * HINT_STATUS_SUGGESTION does it set board->hint_used; it changes no other
 * field of *board (entries, cursor, focus, result, conflicts, history, move
 * count). The suggestion comes from find_answer_completion over the current
 * entries, never from the puzzle's solution fields. Requires
 * board->puzzle != NULL. */
BoardHint request_board_hint(Board* board);

/* False once a hint was shown in this game: solving it then records nothing in
 * the statistics. */
bool board_counts_for_statistics(const Board* board);

#endif
