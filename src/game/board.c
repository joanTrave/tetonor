#include "board.h"
#include "answer_check.h"

static int find_editable_slot(const Board* board, int from, int step);
static void move_cursor(Board* board, int step);
static void type_digit(Board* board, int digit);
static void delete_digit(Board* board);
static void confirm_slot(Board* board);
static void reopen_slot(Board* board, BoardSlot* slot);
static bool all_editable_confirmed(const Board* board);
static void judge_board(Board* board);

void init_board(Board* board, const NumberGeneration* puzzle)
{
    board->puzzle = puzzle;
    board->cursor = TETONOR_NO_CURSOR;
    board->result = BOARD_RESULT_PENDING;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        BoardSlot* slot = &board->slots[i];
        slot->editable  = puzzle->available_array[i] == TETONOR_HIDDEN_NUMBER;
        slot->value     = TETONOR_EMPTY_ENTRY;
        slot->confirmed = false;
    }
    board->cursor = find_editable_slot(board, 0, 1);
}

void apply_board_input(Board* board, BoardInput input)
{
    if (board->cursor == TETONOR_NO_CURSOR)
    {
        return;
    }
    switch (input.kind)
    {
        case BOARD_INPUT_MOVE_LEFT:
            move_cursor(board, -1);
            break;
        case BOARD_INPUT_MOVE_RIGHT:
            move_cursor(board, 1);
            break;
        case BOARD_INPUT_DIGIT:
            type_digit(board, input.digit);
            break;
        case BOARD_INPUT_DELETE:
            delete_digit(board);
            break;
        case BOARD_INPUT_CONFIRM:
            confirm_slot(board);
            break;
    }
}

/* First editable index at or beyond from, walking by step; TETONOR_NO_CURSOR
 * when the walk leaves the row without finding one. */
static int find_editable_slot(const Board* board, int from, int step)
{
    for (int i = from; i >= 0 && i < TETONOR_CELL_COUNT; i += step)
    {
        if (board->slots[i].editable)
        {
            return i;
        }
    }
    return TETONOR_NO_CURSOR;
}

static void move_cursor(Board* board, int step)
{
    int target = find_editable_slot(board, board->cursor + step, step);
    if (target != TETONOR_NO_CURSOR)
    {
        board->cursor = target;
    }
}

static void reopen_slot(Board* board, BoardSlot* slot)
{
    slot->confirmed = false;
    board->result   = BOARD_RESULT_PENDING;
}

static void type_digit(Board* board, int digit)
{
    if (digit < 0 || digit >= TETONOR_DIGIT_BASE)
    {
        return;
    }
    BoardSlot* slot = &board->slots[board->cursor];
    if (slot->confirmed)
    {
        reopen_slot(board, slot);
        slot->value = digit;
        return;
    }
    if (slot->value == TETONOR_EMPTY_ENTRY || slot->value == 0)
    {
        slot->value = digit;
        return;
    }
    int candidate = slot->value * TETONOR_DIGIT_BASE + digit;
    if (candidate <= TETONOR_MAX_VALUE)
    {
        slot->value = candidate;
    }
}

static void delete_digit(Board* board)
{
    BoardSlot* slot = &board->slots[board->cursor];
    if (slot->confirmed)
    {
        reopen_slot(board, slot);
    }
    if (slot->value == TETONOR_EMPTY_ENTRY)
    {
        return;
    }
    slot->value = slot->value < TETONOR_DIGIT_BASE
                      ? TETONOR_EMPTY_ENTRY
                      : slot->value / TETONOR_DIGIT_BASE;
}

static void confirm_slot(Board* board)
{
    BoardSlot* slot = &board->slots[board->cursor];
    if (slot->value == TETONOR_EMPTY_ENTRY || slot->confirmed)
    {
        return;
    }
    slot->confirmed = true;
    if (all_editable_confirmed(board))
    {
        judge_board(board);
    }
}

static bool all_editable_confirmed(const Board* board)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (board->slots[i].editable && !board->slots[i].confirmed)
        {
            return false;
        }
    }
    return true;
}

static void judge_board(Board* board)
{
    int answer[TETONOR_CELL_COUNT];
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        answer[i] = board->slots[i].editable
                        ? board->slots[i].value
                        : board->puzzle->available_array[i];
    }
    board->result = is_answer_correct(board->puzzle, answer)
                        ? BOARD_RESULT_CORRECT
                        : BOARD_RESULT_WRONG;
}
