#ifndef TETONOR_BOARD_H
#define TETONOR_BOARD_H

#include <stdbool.h>

#include "number_generation.h"
#include "operand_group.h"
#include "palette.h"
#include "slot_usage.h"

#define TETONOR_NO_CURSOR (-1) /* board has no editable slot */
#define TETONOR_DIGIT_BASE 10

typedef enum
{
    BOARD_RESULT_PENDING,
    BOARD_RESULT_CORRECT,
    BOARD_RESULT_WRONG
} BoardResult;

typedef enum
{
    BOARD_INPUT_MOVE_LEFT,
    BOARD_INPUT_MOVE_RIGHT,
    BOARD_INPUT_MOVE_UP,
    BOARD_INPUT_MOVE_DOWN,
    BOARD_INPUT_DIGIT,
    BOARD_INPUT_DELETE,
    BOARD_INPUT_ACTIVATE, /* acts on the focused element */
    BOARD_INPUT_OPERATION /* sets the focused operation box */
} BoardInputKind;

typedef struct
{
    BoardInputKind kind;
    int digit;                /* 0-9, used only by BOARD_INPUT_DIGIT */
    GroupOperation operation; /* used only by BOARD_INPUT_OPERATION */
} BoardInput;

typedef enum
{
    BOARD_EVENT_NONE,   /* the input was handled inside the board */
    BOARD_EVENT_RESTART /* the player asked for a new game; the caller acts */
} BoardEvent;

typedef enum
{
    BOARD_FOCUS_SLOT,   /* the slot at cursor */
    BOARD_FOCUS_SUBMIT, /* the control that submits the whole answer */
    BOARD_FOCUS_GROUP,  /* field group_field of groups[group_cursor] */
    BOARD_FOCUS_RESTART /* the control that starts a new game */
} BoardFocus;

typedef struct
{
    bool editable;
    int value; /* TETONOR_EMPTY_ENTRY when nothing typed; unused if !editable */
} BoardSlot;

typedef struct
{
    const NumberGeneration* puzzle; /* borrowed; must outlive the board */
    BoardSlot slots[TETONOR_CELL_COUNT];
    OperandGroup groups[TETONOR_CELL_COUNT]; /* row-major cell index */
    int cursor; /* editable slot index, or TETONOR_NO_CURSOR; remembered while
                   focus is elsewhere */
    int group_cursor; /* cell index; remembered while focus is elsewhere */
    GroupField group_field; /* remembered while focus is elsewhere */
    BoardFocus focus;
    BoardResult result;
    SlotUsage slot_usages[TETONOR_CELL_COUNT]; /* derived from the strip and
                                                  the groups */
    Palette palette; /* every colour of the game; the only instance */
} Board;

/* Starts a fresh entry session over puzzle. Also fills the default palette and
 * the slot usages. Acquires nothing: Board is a fixed-size value, so there is
 * no matching release function. */
void init_board(Board* board, const NumberGeneration* puzzle);

/* Applies one player input. Slot and group focus edit and move; ACTIVATE on
 * the submit control judges the whole answer once every editable slot and
 * every group box is filled. Recomputes slot_usages after every input.
 * ACTIVATE on the restart control returns BOARD_EVENT_RESTART and leaves the
 * board unchanged; every other input returns BOARD_EVENT_NONE. */
BoardEvent apply_board_input(Board* board, BoardInput input);

#endif
