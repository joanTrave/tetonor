#ifndef TETONOR_BOARD_H
#define TETONOR_BOARD_H

#include <stdbool.h>

#include "number_generation.h"
#include "operand_group.h"
#include "palette.h"
#include "slot_usage.h"

#define TETONOR_NO_CURSOR (-1) /* board has no editable slot */
#define TETONOR_DIGIT_BASE 10
#define TETONOR_UNDO_HISTORY_DEPTH                                             \
    20 /* moves kept; one more drops the oldest */

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

/* The part of a board that one move can change, and that undo restores. */
typedef struct
{
    int slot_values[TETONOR_CELL_COUNT]; /* BoardSlot.value of every slot */
    OperandGroup groups[TETONOR_CELL_COUNT];
    BoardResult result;
} BoardSnapshot;

/* The board entries before each of the latest moves, newest on top. A ring
 * buffer: recording into a full history overwrites the oldest entry. */
typedef struct
{
    BoardSnapshot entries[TETONOR_UNDO_HISTORY_DEPTH];
    int newest; /* index of the newest entry; meaningful only when count > 0 */
    int count;  /* 0..TETONOR_UNDO_HISTORY_DEPTH */
} MoveHistory;

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
    Palette palette;     /* every colour of the game; the only instance */
    MoveHistory history; /* this game's undoable moves; emptied by init_board */
} Board;

/* Starts a fresh entry session over puzzle. Also fills the default palette and
 * the slot usages. Empties the move history. Acquires nothing: Board is a
 * fixed-size value, so there is no matching release function. */
void init_board(Board* board, const NumberGeneration* puzzle);

/* Applies one player input. Slot and group focus edit and move; ACTIVATE on
 * the submit control judges the whole answer once every editable slot and
 * every group box is filled. Recomputes slot_usages after every input.
 * Records the entries before every input that changes a slot value or a group
 * box as one undoable move, keeping the latest TETONOR_UNDO_HISTORY_DEPTH.
 * ACTIVATE on the restart control returns BOARD_EVENT_RESTART and leaves the
 * board unchanged; every other input returns BOARD_EVENT_NONE. */
BoardEvent apply_board_input(Board* board, BoardInput input);

/* Reverts the latest move recorded by apply_board_input: slot values, group
 * boxes and result return to what they were just before it, and slot_usages
 * are recomputed. Cursor, group cursor and field, and focus are not touched.
 * Returns false, leaving the board unchanged, when there is no move to undo. */
bool undo_board_move(Board* board);

#endif
