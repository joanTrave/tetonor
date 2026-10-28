#include "answer_search.h"

#include <string.h>

/* An answer is eight cell pairs: an addition cell and a multiplication cell
 * that hold the same ordered operands. The sorted operands of the pairs are
 * the strip. The search assigns cells in pairs and checks the strip at the
 * end. */
typedef struct
{
    int targets[TETONOR_CELL_COUNT];
    const int* fixed;            /* strip; TETONOR_EMPTY_ENTRY = free slot */
    const OperandGroup* entries; /* player's boxes, one per cell */
    int capacity[TETONOR_VALUE_COUNT]; /* most times a value fits the strip */
    int used[TETONOR_VALUE_COUNT];     /* operands placed so far */
    bool assigned[TETONOR_CELL_COUNT];
    OperandGroup groups[TETONOR_CELL_COUNT];
    int operands[TETONOR_CELL_COUNT]; /* two per placed pair, placement order */
    int placed_pairs;
    int completed_strip[TETONOR_CELL_COUNT];
    long nodes;
    bool gave_up;
} SearchState;

typedef struct
{
    int first;
    int last; /* first > last when empty */
} ValueRange;

static bool is_in_range(int value);
static bool is_strip_valid(const int strip[TETONOR_CELL_COUNT]);
static bool are_groups_in_range(const OperandGroup groups[TETONOR_CELL_COUNT]);
static void init_search_state(SearchState* state,
                              const NumberGeneration* puzzle,
                              const int strip[TETONOR_CELL_COUNT],
                              const OperandGroup groups[TETONOR_CELL_COUNT]);
static void compute_capacities(SearchState* state);
static bool search_from(SearchState* state);
static int first_unassigned_cell(const SearchState* state);
static bool is_leaf_consistent(SearchState* state);
static bool try_addition(SearchState* state, int cell);
static bool try_multiplication(SearchState* state, int cell);
static ValueRange multiplication_greaters(int smaller, int target);
static bool try_pair(SearchState* state, int cell, int smaller,
                     GroupOperation operation, int greater);
static bool fits_capacity(const SearchState* state, int smaller, int greater);
static bool matches_entry(const OperandGroup* entry, int smaller,
                          GroupOperation operation, int greater);
static void place_pair(SearchState* state, int first_cell, int second_cell,
                       int smaller, GroupOperation first_operation,
                       int greater);
static void unplace_pair(SearchState* state, int first_cell, int second_cell,
                         int smaller, int greater);

bool find_answer_completion(const NumberGeneration* puzzle,
                            const int strip[TETONOR_CELL_COUNT],
                            const OperandGroup groups[TETONOR_CELL_COUNT],
                            int completed_strip[TETONOR_CELL_COUNT],
                            OperandGroup completed_groups[TETONOR_CELL_COUNT])
{
    if (!is_strip_valid(strip) || !are_groups_in_range(groups))
    {
        return false;
    }
    SearchState state;
    memset(&state, 0, sizeof state);
    init_search_state(&state, puzzle, strip, groups);
    if (!search_from(&state))
    {
        return false;
    }
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        completed_strip[i]  = state.completed_strip[i];
        completed_groups[i] = state.groups[i];
    }
    return true;
}

static bool is_in_range(int value)
{
    return value >= TETONOR_MIN_VALUE && value <= TETONOR_MAX_VALUE;
}

/* Every fixed slot is in range and the fixed values do not decrease. */
static bool is_strip_valid(const int strip[TETONOR_CELL_COUNT])
{
    int previous = TETONOR_MIN_VALUE;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (strip[i] == TETONOR_EMPTY_ENTRY)
        {
            continue;
        }
        if (!is_in_range(strip[i]) || strip[i] < previous)
        {
            return false;
        }
        previous = strip[i];
    }
    return true;
}

static bool are_groups_in_range(const OperandGroup groups[TETONOR_CELL_COUNT])
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        const OperandGroup* group = &groups[i];
        if ((group->smaller != TETONOR_EMPTY_ENTRY &&
             !is_in_range(group->smaller)) ||
            (group->greater != TETONOR_EMPTY_ENTRY &&
             !is_in_range(group->greater)))
        {
            return false;
        }
    }
    return true;
}

static void init_search_state(SearchState* state,
                              const NumberGeneration* puzzle,
                              const int strip[TETONOR_CELL_COUNT],
                              const OperandGroup groups[TETONOR_CELL_COUNT])
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        state->targets[i] =
            puzzle->sum_matrix[i / TETONOR_GRID_SIZE][i % TETONOR_GRID_SIZE];
    }
    state->fixed   = strip;
    state->entries = groups;
    compute_capacities(state);
}

/* A fixed slot holds its own value. A free slot may hold any value between the
 * nearest fixed value on its left and on its right. */
static void compute_capacities(SearchState* state)
{
    int low[TETONOR_CELL_COUNT];
    int high[TETONOR_CELL_COUNT];
    int bound = TETONOR_MIN_VALUE;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        bound =
            state->fixed[i] != TETONOR_EMPTY_ENTRY ? state->fixed[i] : bound;
        low[i] = bound;
    }
    bound = TETONOR_MAX_VALUE;
    for (int i = TETONOR_CELL_COUNT - 1; i >= 0; i--)
    {
        bound =
            state->fixed[i] != TETONOR_EMPTY_ENTRY ? state->fixed[i] : bound;
        high[i] = bound;
    }
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        for (int value = low[i]; value <= high[i]; value++)
        {
            state->capacity[value - TETONOR_MIN_VALUE]++;
        }
    }
}

/* Depth first over the lowest unassigned cell. */
static bool search_from(SearchState* state)
{
    if (++state->nodes > TETONOR_ANSWER_SEARCH_NODE_LIMIT)
    {
        state->gave_up = true;
        return false;
    }
    int cell = first_unassigned_cell(state);
    if (cell == TETONOR_CELL_COUNT)
    {
        return is_leaf_consistent(state);
    }
    return try_addition(state, cell) || try_multiplication(state, cell);
}

static int first_unassigned_cell(const SearchState* state)
{
    int cell = 0;
    while (cell < TETONOR_CELL_COUNT && state->assigned[cell])
    {
        cell++;
    }
    return cell;
}

/* Sorts the placed operands into the strip and compares it with every fixed
 * slot. */
static bool is_leaf_consistent(SearchState* state)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        int value = state->operands[i];
        int at    = i;
        while (at > 0 && state->completed_strip[at - 1] > value)
        {
            state->completed_strip[at] = state->completed_strip[at - 1];
            at--;
        }
        state->completed_strip[at] = value;
    }
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (state->fixed[i] != TETONOR_EMPTY_ENTRY &&
            state->fixed[i] != state->completed_strip[i])
        {
            return false;
        }
    }
    return true;
}

/* The cell is a sum: smaller + greater = target. */
static bool try_addition(SearchState* state, int cell)
{
    int target = state->targets[cell];
    int first  = target - TETONOR_MAX_VALUE;
    if (first < TETONOR_MIN_VALUE)
    {
        first = TETONOR_MIN_VALUE;
    }
    for (int smaller = first; smaller <= target / 2 && !state->gave_up;
         smaller++)
    {
        if (try_pair(state, cell, smaller, GROUP_OPERATION_ADD,
                     target - smaller))
        {
            return true;
        }
    }
    return false;
}

/* The cell is a product: smaller x greater = target. */
static bool try_multiplication(SearchState* state, int cell)
{
    int target = state->targets[cell];
    for (int smaller = TETONOR_MIN_VALUE;
         smaller <= TETONOR_MAX_VALUE && !state->gave_up; smaller++)
    {
        ValueRange greaters = multiplication_greaters(smaller, target);
        for (int greater = greaters.first;
             greater <= greaters.last && !state->gave_up; greater++)
        {
            if (try_pair(state, cell, smaller, GROUP_OPERATION_MULTIPLY,
                         greater))
            {
                return true;
            }
        }
    }
    return false;
}

/* The values greater with smaller <= greater <= TETONOR_MAX_VALUE and
 * smaller x greater = target. */
static ValueRange multiplication_greaters(int smaller, int target)
{
    ValueRange none = {1, 0};
    if (smaller == 0)
    {
        ValueRange any = {smaller, TETONOR_MAX_VALUE};
        return target == 0 ? any : none;
    }
    if (target % smaller != 0)
    {
        return none;
    }
    int greater = target / smaller;
    if (greater < smaller || greater > TETONOR_MAX_VALUE)
    {
        return none;
    }
    ValueRange only = {greater, greater};
    return only;
}

/* Gives the cell the pair, then looks for the other cell of the pair: an
 * unassigned cell whose target is what the opposite operation makes of it. */
static bool try_pair(SearchState* state, int cell, int smaller,
                     GroupOperation operation, int greater)
{
    if (!matches_entry(&state->entries[cell], smaller, operation, greater) ||
        !fits_capacity(state, smaller, greater))
    {
        return false;
    }
    bool is_sum = operation == GROUP_OPERATION_ADD;
    GroupOperation partner_operation =
        is_sum ? GROUP_OPERATION_MULTIPLY : GROUP_OPERATION_ADD;
    int partner_target = is_sum ? smaller * greater : smaller + greater;
    for (int other = cell + 1; other < TETONOR_CELL_COUNT && !state->gave_up;
         other++)
    {
        if (state->assigned[other] || state->targets[other] != partner_target ||
            !matches_entry(&state->entries[other], smaller, partner_operation,
                           greater))
        {
            continue;
        }
        place_pair(state, cell, other, smaller, operation, greater);
        if (search_from(state))
        {
            return true;
        }
        unplace_pair(state, cell, other, smaller, greater);
    }
    return false;
}

/* Whether one more pair of smaller and greater still fits the strip. */
static bool fits_capacity(const SearchState* state, int smaller, int greater)
{
    int s = smaller - TETONOR_MIN_VALUE;
    int g = greater - TETONOR_MIN_VALUE;
    if (smaller == greater)
    {
        return state->used[s] + 2 <= state->capacity[s];
    }
    return state->used[s] + 1 <= state->capacity[s] &&
           state->used[g] + 1 <= state->capacity[g];
}

/* Whether the pair agrees with every box the player filled. */
static bool matches_entry(const OperandGroup* entry, int smaller,
                          GroupOperation operation, int greater)
{
    return (entry->smaller == TETONOR_EMPTY_ENTRY ||
            entry->smaller == smaller) &&
           (entry->operation == GROUP_OPERATION_NONE ||
            entry->operation == operation) &&
           (entry->greater == TETONOR_EMPTY_ENTRY || entry->greater == greater);
}

static void place_pair(SearchState* state, int first_cell, int second_cell,
                       int smaller, GroupOperation first_operation, int greater)
{
    GroupOperation second_operation = first_operation == GROUP_OPERATION_ADD
                                          ? GROUP_OPERATION_MULTIPLY
                                          : GROUP_OPERATION_ADD;
    state->assigned[first_cell]     = true;
    state->assigned[second_cell]    = true;
    state->groups[first_cell] =
        (OperandGroup){smaller, first_operation, greater};
    state->groups[second_cell] =
        (OperandGroup){smaller, second_operation, greater};
    state->operands[2 * state->placed_pairs]     = smaller;
    state->operands[2 * state->placed_pairs + 1] = greater;
    state->placed_pairs++;
    state->used[smaller - TETONOR_MIN_VALUE]++;
    state->used[greater - TETONOR_MIN_VALUE]++;
}

static void unplace_pair(SearchState* state, int first_cell, int second_cell,
                         int smaller, int greater)
{
    state->assigned[first_cell]  = false;
    state->assigned[second_cell] = false;
    state->placed_pairs--;
    state->used[smaller - TETONOR_MIN_VALUE]--;
    state->used[greater - TETONOR_MIN_VALUE]--;
}
