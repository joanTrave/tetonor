#include "hint.h"
#include "answer_search.h"

typedef struct
{
    bool found;
    HintTarget kind;
    int index;
} HintPlace;

static bool is_slot_open(const Board* board, int slot);
static HintPlace place_slot(int slot);
static HintPlace place_group(int cell);
static HintPlace no_place(void);
static int first_open_group(const Board* board);
static int first_open_slot(const Board* board);
static HintPlace choose_hint_place(const Board* board);
static BoardHint make_suggestion(HintPlace place,
                                 const int strip[TETONOR_CELL_COUNT],
                                 const OperandGroup groups[TETONOR_CELL_COUNT]);
static BoardHint make_reply(HintStatus status);

BoardHint request_board_hint(Board* board)
{
    if (is_board_locked(board))
    {
        return make_reply(HINT_STATUS_SOLVED);
    }
    if (board->conflicts.count > 0)
    {
        return make_reply(HINT_STATUS_CONFLICTS);
    }
    HintPlace place = choose_hint_place(board);
    if (!place.found)
    {
        return make_reply(HINT_STATUS_BOARD_FULL);
    }
    int strip[TETONOR_CELL_COUNT];
    int completed_strip[TETONOR_CELL_COUNT];
    OperandGroup completed_groups[TETONOR_CELL_COUNT];
    build_board_strip(board, strip);
    if (!find_answer_completion(board->puzzle, strip, board->groups,
                                completed_strip, completed_groups))
    {
        return make_reply(HINT_STATUS_NO_SUGGESTION);
    }
    board->hint_used = true;
    return make_suggestion(place, completed_strip, completed_groups);
}

bool board_counts_for_statistics(const Board* board)
{
    return !board->hint_used;
}

static BoardHint make_reply(HintStatus status)
{
    BoardHint hint = {0};
    hint.status    = status;
    return hint;
}

static BoardHint make_suggestion(HintPlace place,
                                 const int strip[TETONOR_CELL_COUNT],
                                 const OperandGroup groups[TETONOR_CELL_COUNT])
{
    BoardHint hint = make_reply(HINT_STATUS_SUGGESTION);
    hint.target    = place.kind;
    hint.index     = place.index;
    if (place.kind == HINT_TARGET_SLOT)
    {
        hint.value = strip[place.index];
    }
    else
    {
        hint.group = groups[place.index];
    }
    return hint;
}

static bool is_slot_open(const Board* board, int slot)
{
    return board->slots[slot].editable &&
           board->slots[slot].value == TETONOR_EMPTY_ENTRY;
}

static HintPlace place_slot(int slot)
{
    HintPlace place = {true, HINT_TARGET_SLOT, slot};
    return place;
}

static HintPlace place_group(int cell)
{
    HintPlace place = {true, HINT_TARGET_GROUP, cell};
    return place;
}

static HintPlace no_place(void)
{
    HintPlace place = {false, HINT_TARGET_SLOT, 0};
    return place;
}

/* TETONOR_CELL_COUNT when every group is filled. */
static int first_open_group(const Board* board)
{
    int cell = 0;
    while (cell < TETONOR_CELL_COUNT &&
           is_operand_group_filled(&board->groups[cell]))
    {
        cell++;
    }
    return cell;
}

/* TETONOR_CELL_COUNT when every editable slot is typed. */
static int first_open_slot(const Board* board)
{
    int slot = 0;
    while (slot < TETONOR_CELL_COUNT && !is_slot_open(board, slot))
    {
        slot++;
    }
    return slot;
}

static HintPlace choose_hint_place(const Board* board)
{
    if (board->focus == BOARD_FOCUS_SLOT &&
        board->cursor != TETONOR_NO_CURSOR &&
        is_slot_open(board, board->cursor))
    {
        return place_slot(board->cursor);
    }
    if (board->focus == BOARD_FOCUS_GROUP &&
        !is_operand_group_filled(&board->groups[board->group_cursor]))
    {
        return place_group(board->group_cursor);
    }
    int cell = first_open_group(board);
    if (cell < TETONOR_CELL_COUNT)
    {
        return place_group(cell);
    }
    int slot = first_open_slot(board);
    return slot < TETONOR_CELL_COUNT ? place_slot(slot) : no_place();
}
