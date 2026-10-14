#ifndef TETONOR_BOARD_H
#define TETONOR_BOARD_H

#include <stdbool.h>

#include "number_generation.h"

#define TETONOR_EMPTY_ENTRY (-1) /* editable slot with nothing typed */
#define TETONOR_NO_CURSOR (-1)   /* board has no editable slot */
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
    BOARD_INPUT_ACTIVATE /* acts on the focused element */
} BoardInputKind;

typedef struct
{
    BoardInputKind kind;
    int digit; /* 0-9, used only by BOARD_INPUT_DIGIT */
} BoardInput;

typedef enum
{
    BOARD_FOCUS_SLOT,  /* the slot at cursor */
    BOARD_FOCUS_SUBMIT /* the control that submits the whole answer */
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
    int cursor; /* editable slot index, or TETONOR_NO_CURSOR; kept while focus
                   is BOARD_FOCUS_SUBMIT */
    BoardFocus focus;
    BoardResult result;
} Board;

/* Starts a fresh entry session over puzzle. Acquires nothing: Board is a
 * fixed-size value, so there is no matching release function. */
void init_board(Board* board, const NumberGeneration* puzzle);

/* Applies one player input. Slot focus edits and moves; ACTIVATE on the submit
 * control judges the whole answer once every editable slot is filled. */
void apply_board_input(Board* board, BoardInput input);

#endif
