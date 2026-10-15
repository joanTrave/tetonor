#include "board.h"
#include "answer_check.h"

#include <stddef.h>

static void apply_slot_input(Board* board, BoardInput input);
static void apply_group_input(Board* board, BoardInput input);
static void apply_submit_input(Board* board, BoardInput input);
static BoardEvent apply_restart_input(Board* board, BoardInput input);
static int find_editable_slot(const Board* board, int from, int step);
static bool move_cursor(Board* board, int step);
static void move_group_horizontally(Board* board, int step);
static bool move_group_vertically(Board* board, int step);
static void leave_groups_downward(Board* board);
static void leave_submit(Board* board);
static void focus_submit(Board* board);
static void focus_restart(Board* board);
static int* focused_group_number(Board* board);
static void set_operation(Board* board, GroupOperation operation);
static void clear_operation(Board* board);
static void type_digit(Board* board, int* value, int digit);
static void delete_digit(Board* board, int* value);
static void clear_result(Board* board);
static bool all_editable_filled(const Board* board);
static bool all_groups_filled(const Board* board);
static void submit_answer(Board* board);
static void judge_board(Board* board);
static void build_strip(const Board* board, int strip[TETONOR_CELL_COUNT]);
static void refresh_slot_usages(Board* board);

void init_board(Board* board, const NumberGeneration* puzzle)
{
    board->puzzle       = puzzle;
    board->cursor       = TETONOR_NO_CURSOR;
    board->result       = BOARD_RESULT_PENDING;
    board->group_cursor = 0;
    board->group_field  = GROUP_FIELD_SMALLER;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        BoardSlot* slot = &board->slots[i];
        slot->editable  = puzzle->available_array[i] == TETONOR_HIDDEN_NUMBER;
        slot->value     = TETONOR_EMPTY_ENTRY;
        clear_operand_group(&board->groups[i]);
    }
    board->cursor = find_editable_slot(board, 0, 1);
    board->focus  = board->cursor != TETONOR_NO_CURSOR ? BOARD_FOCUS_SLOT
                                                       : BOARD_FOCUS_GROUP;
    init_palette(&board->palette);
    refresh_slot_usages(board);
}

BoardEvent apply_board_input(Board* board, BoardInput input)
{
    BoardEvent event = BOARD_EVENT_NONE;
    switch (board->focus)
    {
        case BOARD_FOCUS_SLOT:
            apply_slot_input(board, input);
            break;
        case BOARD_FOCUS_GROUP:
            apply_group_input(board, input);
            break;
        case BOARD_FOCUS_SUBMIT:
            apply_submit_input(board, input);
            break;
        case BOARD_FOCUS_RESTART:
            event = apply_restart_input(board, input);
            break;
    }
    refresh_slot_usages(board);
    return event;
}

static void apply_slot_input(Board* board, BoardInput input)
{
    int* value = &board->slots[board->cursor].value;
    switch (input.kind)
    {
        case BOARD_INPUT_MOVE_LEFT:
            move_cursor(board, -1);
            break;
        case BOARD_INPUT_MOVE_RIGHT:
            if (!move_cursor(board, 1))
            {
                focus_submit(board);
            }
            break;
        case BOARD_INPUT_MOVE_UP:
            board->focus = BOARD_FOCUS_GROUP;
            break;
        case BOARD_INPUT_MOVE_DOWN:
            focus_submit(board);
            break;
        case BOARD_INPUT_DIGIT:
            type_digit(board, value, input.digit);
            break;
        case BOARD_INPUT_DELETE:
            delete_digit(board, value);
            break;
        case BOARD_INPUT_ACTIVATE:
        case BOARD_INPUT_OPERATION:
            break;
    }
}

static void apply_group_input(Board* board, BoardInput input)
{
    int* number = focused_group_number(board);
    switch (input.kind)
    {
        case BOARD_INPUT_MOVE_LEFT:
            move_group_horizontally(board, -1);
            break;
        case BOARD_INPUT_MOVE_RIGHT:
            move_group_horizontally(board, 1);
            break;
        case BOARD_INPUT_MOVE_UP:
            move_group_vertically(board, -1);
            break;
        case BOARD_INPUT_MOVE_DOWN:
            if (!move_group_vertically(board, 1))
            {
                leave_groups_downward(board);
            }
            break;
        case BOARD_INPUT_DIGIT:
            if (number != NULL)
            {
                type_digit(board, number, input.digit);
            }
            break;
        case BOARD_INPUT_DELETE:
            if (number != NULL)
            {
                delete_digit(board, number);
            }
            else
            {
                clear_operation(board);
            }
            break;
        case BOARD_INPUT_OPERATION:
            if (number == NULL)
            {
                set_operation(board, input.operation);
            }
            break;
        case BOARD_INPUT_ACTIVATE:
            break;
    }
}

static void apply_submit_input(Board* board, BoardInput input)
{
    switch (input.kind)
    {
        case BOARD_INPUT_MOVE_UP:
        case BOARD_INPUT_MOVE_LEFT:
            leave_submit(board);
            break;
        case BOARD_INPUT_ACTIVATE:
            submit_answer(board);
            break;
        case BOARD_INPUT_MOVE_DOWN:
            focus_restart(board);
            break;
        case BOARD_INPUT_MOVE_RIGHT:
        case BOARD_INPUT_DIGIT:
        case BOARD_INPUT_DELETE:
        case BOARD_INPUT_OPERATION:
            break;
    }
}

static BoardEvent apply_restart_input(Board* board, BoardInput input)
{
    switch (input.kind)
    {
        case BOARD_INPUT_MOVE_UP:
            focus_submit(board);
            break;
        case BOARD_INPUT_MOVE_LEFT:
            leave_submit(board);
            break;
        case BOARD_INPUT_ACTIVATE:
            return BOARD_EVENT_RESTART;
        case BOARD_INPUT_MOVE_RIGHT:
        case BOARD_INPUT_MOVE_DOWN:
        case BOARD_INPUT_DIGIT:
        case BOARD_INPUT_DELETE:
        case BOARD_INPUT_OPERATION:
            break;
    }
    return BOARD_EVENT_NONE;
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

/* Returns whether the cursor moved. */
static bool move_cursor(Board* board, int step)
{
    int target = find_editable_slot(board, board->cursor + step, step);
    if (target == TETONOR_NO_CURSOR)
    {
        return false;
    }
    board->cursor = target;
    return true;
}

/* Steps through the fields of a group, then into the neighbouring group of the
 * same matrix row. Never wraps across rows. */
static void move_group_horizontally(Board* board, int step)
{
    int field = (int)board->group_field + step;
    if (field >= GROUP_FIELD_SMALLER && field <= GROUP_FIELD_GREATER)
    {
        board->group_field = (GroupField)field;
        return;
    }
    int col = board->group_cursor % TETONOR_GRID_SIZE + step;
    if (col < 0 || col >= TETONOR_GRID_SIZE)
    {
        return;
    }
    board->group_cursor += step;
    board->group_field = step > 0 ? GROUP_FIELD_SMALLER : GROUP_FIELD_GREATER;
}

/* Returns whether the group cursor moved. */
static bool move_group_vertically(Board* board, int step)
{
    int row = board->group_cursor / TETONOR_GRID_SIZE + step;
    if (row < 0 || row >= TETONOR_GRID_SIZE)
    {
        return false;
    }
    board->group_cursor += step * TETONOR_GRID_SIZE;
    return true;
}

static void leave_groups_downward(Board* board)
{
    if (board->cursor != TETONOR_NO_CURSOR)
    {
        board->focus = BOARD_FOCUS_SLOT;
        return;
    }
    focus_submit(board);
}

static void leave_submit(Board* board)
{
    board->focus = board->cursor != TETONOR_NO_CURSOR ? BOARD_FOCUS_SLOT
                                                      : BOARD_FOCUS_GROUP;
}

static void focus_submit(Board* board)
{
    board->focus = BOARD_FOCUS_SUBMIT;
}

static void focus_restart(Board* board)
{
    board->focus = BOARD_FOCUS_RESTART;
}

/* The focused group's number box, or NULL when the operation box is focused. */
static int* focused_group_number(Board* board)
{
    OperandGroup* group = &board->groups[board->group_cursor];
    switch (board->group_field)
    {
        case GROUP_FIELD_SMALLER:
            return &group->smaller;
        case GROUP_FIELD_GREATER:
            return &group->greater;
        case GROUP_FIELD_OPERATION:
            break;
    }
    return NULL;
}

static void clear_result(Board* board)
{
    board->result = BOARD_RESULT_PENDING;
}

static void set_operation(Board* board, GroupOperation operation)
{
    if (operation == GROUP_OPERATION_NONE)
    {
        return;
    }
    clear_result(board);
    board->groups[board->group_cursor].operation = operation;
}

static void clear_operation(Board* board)
{
    clear_result(board);
    board->groups[board->group_cursor].operation = GROUP_OPERATION_NONE;
}

static void type_digit(Board* board, int* value, int digit)
{
    if (digit < 0 || digit >= TETONOR_DIGIT_BASE)
    {
        return;
    }
    if (board->result != BOARD_RESULT_PENDING)
    {
        clear_result(board);
        *value = digit;
        return;
    }
    if (*value == TETONOR_EMPTY_ENTRY || *value == 0)
    {
        *value = digit;
        return;
    }
    int candidate = *value * TETONOR_DIGIT_BASE + digit;
    if (candidate <= TETONOR_MAX_VALUE)
    {
        *value = candidate;
    }
}

static void delete_digit(Board* board, int* value)
{
    clear_result(board);
    if (*value == TETONOR_EMPTY_ENTRY)
    {
        return;
    }
    *value = *value < TETONOR_DIGIT_BASE ? TETONOR_EMPTY_ENTRY
                                         : *value / TETONOR_DIGIT_BASE;
}

static bool all_editable_filled(const Board* board)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (board->slots[i].editable &&
            board->slots[i].value == TETONOR_EMPTY_ENTRY)
        {
            return false;
        }
    }
    return true;
}

static bool all_groups_filled(const Board* board)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (!is_operand_group_filled(&board->groups[i]))
        {
            return false;
        }
    }
    return true;
}

static void submit_answer(Board* board)
{
    if (all_editable_filled(board) && all_groups_filled(board))
    {
        judge_board(board);
    }
}

/* The available row as the player sees it: given numbers plus typed ones. */
static void build_strip(const Board* board, int strip[TETONOR_CELL_COUNT])
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        strip[i] = board->slots[i].editable ? board->slots[i].value
                                            : board->puzzle->available_array[i];
    }
}

static void refresh_slot_usages(Board* board)
{
    int strip[TETONOR_CELL_COUNT];
    build_strip(board, strip);
    compute_slot_usages(strip, board->groups, board->slot_usages);
}

static void judge_board(Board* board)
{
    int strip[TETONOR_CELL_COUNT];
    build_strip(board, strip);
    board->result = is_answer_correct(board->puzzle, strip, board->groups)
                        ? BOARD_RESULT_CORRECT
                        : BOARD_RESULT_WRONG;
}
