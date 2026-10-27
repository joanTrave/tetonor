#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../test_harness.h"
#include "fixture_puzzle.h"
#include "../../src/game/answer_check.h"
#include "../../src/game/conflict_check.h"

#define TEST_SEED_COUNT 30
#define TEST_PERIOD_COUNT 4
#define TEST_POISON_COUNT 99
#define TEST_FIELD_COUNT 3
#define TEST_TYPED_SLOT 5
#define TEST_NEXT_TYPED_SLOT 6
#define TEST_WINDOW_SLOT 4
#define TEST_FAR_SLOT 15
#define TEST_BIG_TYPED_VALUE 20
#define TEST_OUT_OF_RANGE_HIGH (TETONOR_MAX_VALUE + 1)
#define TEST_OUT_OF_RANGE_LOW (-2)
#define TEST_SHARED_VALUE 5 /* given once in F-A, at index 5 */
#define TEST_SCRAMBLE_ROUNDS 300
#define TEST_LCG_MULTIPLIER 1103515245u
#define TEST_LCG_INCREMENT 12345u
#define TEST_LCG_SHIFT 16
#define TEST_PAIR_SMALLER 3
#define TEST_PAIR_GREATER 5
#define TEST_PAIR_SUM 8
#define TEST_PAIR_PRODUCT 15
#define TEST_UNIQUE_TARGET_BASE 100

static const int pair_strip[TETONOR_CELL_COUNT] = {
    3, 3, 5, 5, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21};
static const int scramble_values[] = {
    TETONOR_EMPTY_ENTRY,  0, 1, 2, 3, 5, 7, 12, 15, 29, TEST_OUT_OF_RANGE_HIGH,
    TEST_OUT_OF_RANGE_LOW};
static const GroupOperation scramble_operations[] = {
    GROUP_OPERATION_NONE, GROUP_OPERATION_ADD, GROUP_OPERATION_MULTIPLY};
static const int scramble_hidden[] = {2, 5, 6, 15};

typedef struct
{
    NumberGeneration puzzle;
    int strip[TETONOR_CELL_COUNT];
    OperandGroup groups[TETONOR_CELL_COUNT];
    BoardConflicts conflicts;
} ConflictCase;

static void poison_conflicts(BoardConflicts* conflicts)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        conflicts->slots[i] = true;
        for (int field = 0; field < TEST_FIELD_COUNT; field++)
        {
            conflicts->fields[i][field] = true;
        }
    }
    conflicts->count = TEST_POISON_COUNT;
}

static void strip_from_puzzle(const NumberGeneration* puzzle,
                              int strip[TETONOR_CELL_COUNT])
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        strip[i] = puzzle->available_array[i] == TETONOR_HIDDEN_NUMBER
                       ? TETONOR_EMPTY_ENTRY
                       : puzzle->available_array[i];
    }
}

static void empty_groups(OperandGroup groups[TETONOR_CELL_COUNT])
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        clear_operand_group(&groups[i]);
    }
}

/* F-A with the given available-row indexes hidden and empty, every group
 * empty. */
static void start_case(ConflictCase* conflict_case, const int* hidden,
                       size_t hidden_count)
{
    build_fixture_puzzle(&conflict_case->puzzle, hidden, hidden_count);
    strip_from_puzzle(&conflict_case->puzzle, conflict_case->strip);
    empty_groups(conflict_case->groups);
    poison_conflicts(&conflict_case->conflicts);
}

/* F-P: cells with a target of TEST_PAIR_SUM first, then cells with
 * TEST_PAIR_PRODUCT, then cells with targets that occur nowhere else. The
 * strip is all given and holds two 3 and two 5. */
static void start_pair_case(ConflictCase* conflict_case, int sum_cells,
                            int product_cells)
{
    memset(&conflict_case->puzzle, 0, sizeof conflict_case->puzzle);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        int target = TEST_UNIQUE_TARGET_BASE + i;
        if (i < sum_cells)
        {
            target = TEST_PAIR_SUM;
        }
        else if (i < sum_cells + product_cells)
        {
            target = TEST_PAIR_PRODUCT;
        }
        conflict_case->puzzle
            .sum_matrix[i / TETONOR_GRID_SIZE][i % TETONOR_GRID_SIZE] = target;
        conflict_case->puzzle.available_array[i] = pair_strip[i];
        conflict_case->strip[i]                  = pair_strip[i];
    }
    empty_groups(conflict_case->groups);
    poison_conflicts(&conflict_case->conflicts);
}

static void run_case(ConflictCase* conflict_case)
{
    poison_conflicts(&conflict_case->conflicts);
    find_board_conflicts(&conflict_case->puzzle, conflict_case->strip,
                         conflict_case->groups, &conflict_case->conflicts);
}

static void set_group(ConflictCase* conflict_case, int cell, int smaller,
                      GroupOperation operation, int greater)
{
    OperandGroup* group = &conflict_case->groups[cell];
    group->smaller      = smaller;
    group->operation    = operation;
    group->greater      = greater;
}

static bool field_flagged(const ConflictCase* conflict_case, int cell,
                          GroupField field)
{
    return conflict_case->conflicts.fields[cell][field];
}

static bool whole_group_flagged(const ConflictCase* conflict_case, int cell)
{
    return field_flagged(conflict_case, cell, GROUP_FIELD_SMALLER) &&
           field_flagged(conflict_case, cell, GROUP_FIELD_OPERATION) &&
           field_flagged(conflict_case, cell, GROUP_FIELD_GREATER);
}

static int flagged_box_count(const BoardConflicts* conflicts)
{
    int total = 0;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        total += conflicts->slots[i] ? 1 : 0;
        for (int field = 0; field < TEST_FIELD_COUNT; field++)
        {
            total += conflicts->fields[i][field] ? 1 : 0;
        }
    }
    return total;
}

static bool nothing_flagged(const ConflictCase* conflict_case)
{
    return conflict_case->conflicts.count == 0 &&
           flagged_box_count(&conflict_case->conflicts) == 0;
}

/* The count the player sees: flagged slots plus groups with a flagged box. */
static int expected_count(const BoardConflicts* conflicts)
{
    int count = 0;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        count += conflicts->slots[i] ? 1 : 0;
        bool group_flagged = false;
        for (int field = 0; field < TEST_FIELD_COUNT; field++)
        {
            group_flagged = group_flagged || conflicts->fields[i][field];
        }
        count += group_flagged ? 1 : 0;
    }
    return count;
}

static bool empty_boxes_unflagged(const ConflictCase* conflict_case)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        const OperandGroup* group = &conflict_case->groups[i];
        const bool* fields        = conflict_case->conflicts.fields[i];
        if ((conflict_case->strip[i] == TETONOR_EMPTY_ENTRY &&
             conflict_case->conflicts.slots[i]) ||
            (group->smaller == TETONOR_EMPTY_ENTRY &&
             fields[GROUP_FIELD_SMALLER]) ||
            (group->operation == GROUP_OPERATION_NONE &&
             fields[GROUP_FIELD_OPERATION]) ||
            (group->greater == TETONOR_EMPTY_ENTRY &&
             fields[GROUP_FIELD_GREATER]))
        {
            return false;
        }
    }
    return true;
}

static bool given_slots_unflagged(const ConflictCase* conflict_case)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (conflict_case->puzzle.available_array[i] != TETONOR_HIDDEN_NUMBER &&
            conflict_case->conflicts.slots[i])
        {
            return false;
        }
    }
    return true;
}

/* Properties of every result, whatever the board: no empty box and no given
 * slot is flagged, and count follows the flags. */
static bool result_is_well_formed(const ConflictCase* conflict_case)
{
    return empty_boxes_unflagged(conflict_case) &&
           given_slots_unflagged(conflict_case) &&
           conflict_case->conflicts.count ==
               expected_count(&conflict_case->conflicts) &&
           conflict_case->conflicts.count <= TETONOR_CONFLICT_COUNT_MAX;
}

static bool conflicts_equal(const BoardConflicts* first,
                            const BoardConflicts* second)
{
    if (first->count != second->count)
    {
        return false;
    }
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (first->slots[i] != second->slots[i])
        {
            return false;
        }
        for (int field = 0; field < TEST_FIELD_COUNT; field++)
        {
            if (first->fields[i][field] != second->fields[i][field])
            {
                return false;
            }
        }
    }
    return true;
}

/* Blanks every box whose index (cell * 3 + field) is a multiple of period. */
static void blank_boxes(OperandGroup groups[TETONOR_CELL_COUNT], int period)
{
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        OperandGroup* group = &groups[cell];
        if ((cell * TEST_FIELD_COUNT + GROUP_FIELD_SMALLER) % period == 0)
        {
            group->smaller = TETONOR_EMPTY_ENTRY;
        }
        if ((cell * TEST_FIELD_COUNT + GROUP_FIELD_OPERATION) % period == 0)
        {
            group->operation = GROUP_OPERATION_NONE;
        }
        if ((cell * TEST_FIELD_COUNT + GROUP_FIELD_GREATER) % period == 0)
        {
            group->greater = TETONOR_EMPTY_ENTRY;
        }
    }
}

static unsigned int next_random(unsigned int* state)
{
    *state = *state * TEST_LCG_MULTIPLIER + TEST_LCG_INCREMENT;
    return *state >> TEST_LCG_SHIFT;
}

static int random_scramble_value(unsigned int* state)
{
    return scramble_values[next_random(state) % (sizeof scramble_values /
                                                 sizeof scramble_values[0])];
}

static void test_conflict_constants(void)
{
    ASSERT_EQ_INT(TETONOR_GROUP_FIELD_COUNT, TEST_FIELD_COUNT);
    ASSERT_EQ_INT(TETONOR_CONFLICT_COUNT_MAX, 2 * TETONOR_CELL_COUNT);
}

static void test_find_board_conflicts_solved_fixture_has_none(void)
{
    ConflictCase conflict_case;
    start_case(&conflict_case, NULL, 0);
    memcpy(conflict_case.strip, fixture_strip, sizeof conflict_case.strip);
    memcpy(conflict_case.groups, fixture_groups, sizeof conflict_case.groups);
    run_case(&conflict_case);
    ASSERT_TRUE(nothing_flagged(&conflict_case));
}

static void
test_find_board_conflicts_solved_fixture_with_typed_slots_has_none(void)
{
    const int hidden[] = {2, TEST_TYPED_SLOT, TEST_NEXT_TYPED_SLOT,
                          TEST_FAR_SLOT};
    ConflictCase conflict_case;
    start_case(&conflict_case, hidden, sizeof hidden / sizeof hidden[0]);
    memcpy(conflict_case.strip, fixture_strip, sizeof conflict_case.strip);
    memcpy(conflict_case.groups, fixture_groups, sizeof conflict_case.groups);
    run_case(&conflict_case);
    ASSERT_TRUE(nothing_flagged(&conflict_case));
}

static void test_find_board_conflicts_generated_solutions_have_none(void)
{
    for (int seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        for (int difficulty = 0; difficulty < DIFFICULTY_COUNT; difficulty++)
        {
            srand((unsigned int)seed);
            NumberGeneration* puzzle = generate_random_numbers(difficulty);
            ASSERT_NOT_NULL(puzzle);
            BoardConflicts conflicts;
            poison_conflicts(&conflicts);
            find_board_conflicts(puzzle, puzzle->solution_array,
                                 puzzle->solution_groups, &conflicts);
            bool accepted = is_answer_correct(puzzle, puzzle->solution_array,
                                              puzzle->solution_groups);
            int flagged   = flagged_box_count(&conflicts);
            free_number_generation(puzzle);
            ASSERT_TRUE(accepted);
            ASSERT_EQ_INT(conflicts.count, 0);
            ASSERT_EQ_INT(flagged, 0);
        }
    }
}

/* Soundness: a board that is part of a valid solution can still be completed,
 * so it shows no conflict however many boxes are still empty. */
static void test_find_board_conflicts_partial_solutions_have_none(void)
{
    for (int seed = 1; seed <= TEST_SEED_COUNT; seed++)
    {
        for (int difficulty = 0; difficulty < DIFFICULTY_COUNT; difficulty++)
        {
            for (int period = 1; period <= TEST_PERIOD_COUNT; period++)
            {
                srand((unsigned int)seed);
                NumberGeneration* puzzle = generate_random_numbers(difficulty);
                ASSERT_NOT_NULL(puzzle);
                int strip[TETONOR_CELL_COUNT];
                OperandGroup groups[TETONOR_CELL_COUNT];
                BoardConflicts conflicts;
                memcpy(groups, puzzle->solution_groups, sizeof groups);
                blank_boxes(groups, period);
                for (int typed = 0; typed < 2; typed++)
                {
                    strip_from_puzzle(puzzle, strip);
                    if (typed == 1)
                    {
                        memcpy(strip, puzzle->solution_array, sizeof strip);
                    }
                    poison_conflicts(&conflicts);
                    find_board_conflicts(puzzle, strip, groups, &conflicts);
                    int flagged = flagged_box_count(&conflicts);
                    int count   = conflicts.count;
                    if (flagged != 0 || count != 0)
                    {
                        free_number_generation(puzzle);
                        ASSERT_EQ_INT(flagged, 0);
                        ASSERT_EQ_INT(count, 0);
                    }
                }
                free_number_generation(puzzle);
            }
        }
    }
}

static void test_find_board_conflicts_empty_board_has_none(void)
{
    const int hidden[] = {2, TEST_TYPED_SLOT, TEST_NEXT_TYPED_SLOT,
                          TEST_FAR_SLOT};
    ConflictCase conflict_case;
    start_case(&conflict_case, hidden, sizeof hidden / sizeof hidden[0]);
    run_case(&conflict_case);
    ASSERT_TRUE(nothing_flagged(&conflict_case));
}

static void
test_find_board_conflicts_partial_group_is_not_checked_for_target(void)
{
    const int hidden[] = {TEST_FAR_SLOT};
    ConflictCase conflict_case;
    start_case(&conflict_case, hidden, 1);
    set_group(&conflict_case, 0, TEST_BIG_TYPED_VALUE, GROUP_OPERATION_ADD,
              TETONOR_EMPTY_ENTRY);
    set_group(&conflict_case, 1, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_MULTIPLY,
              7);
    set_group(&conflict_case, 2, 1, GROUP_OPERATION_NONE, 2);
    run_case(&conflict_case);
    ASSERT_TRUE(nothing_flagged(&conflict_case));
}

static void test_find_board_conflicts_target_mismatch_flags_whole_group(void)
{
    ConflictCase conflict_case;
    start_case(&conflict_case, NULL, 0);
    set_group(&conflict_case, 0, 1, GROUP_OPERATION_ADD, 3);
    run_case(&conflict_case);
    ASSERT_TRUE(whole_group_flagged(&conflict_case, 0));
    ASSERT_EQ_INT(conflict_case.conflicts.count, 1);
    ASSERT_EQ_INT(flagged_box_count(&conflict_case.conflicts),
                  TEST_FIELD_COUNT);
    ASSERT_TRUE(group_has_conflict(&conflict_case.conflicts, 0));
    ASSERT_TRUE(result_is_well_formed(&conflict_case));
}

static void test_find_board_conflicts_matching_group_is_not_flagged(void)
{
    ConflictCase conflict_case;
    start_case(&conflict_case, NULL, 0);
    set_group(&conflict_case, 0, 1, GROUP_OPERATION_ADD, 2);
    set_group(&conflict_case, 1, 1, GROUP_OPERATION_MULTIPLY, 2);
    run_case(&conflict_case);
    ASSERT_TRUE(nothing_flagged(&conflict_case));
}

static void test_find_board_conflicts_wrong_operation_is_flagged(void)
{
    ConflictCase conflict_case;
    start_case(&conflict_case, NULL, 0);
    set_group(&conflict_case, 0, 1, GROUP_OPERATION_MULTIPLY, 2);
    run_case(&conflict_case);
    ASSERT_TRUE(whole_group_flagged(&conflict_case, 0));
    ASSERT_EQ_INT(conflict_case.conflicts.count, 1);
}

static void test_find_board_conflicts_order_flags_both_numbers(void)
{
    ConflictCase conflict_case;
    start_case(&conflict_case, NULL, 0);
    set_group(&conflict_case, 0, TEST_SHARED_VALUE, GROUP_OPERATION_NONE, 2);
    run_case(&conflict_case);
    ASSERT_TRUE(field_flagged(&conflict_case, 0, GROUP_FIELD_SMALLER));
    ASSERT_TRUE(field_flagged(&conflict_case, 0, GROUP_FIELD_GREATER));
    ASSERT_FALSE(field_flagged(&conflict_case, 0, GROUP_FIELD_OPERATION));
    ASSERT_EQ_INT(conflict_case.conflicts.count, 1);
    ASSERT_TRUE(result_is_well_formed(&conflict_case));
}

static void
test_find_board_conflicts_order_with_operation_flags_whole_group(void)
{
    ConflictCase conflict_case;
    start_case(&conflict_case, NULL, 0);
    set_group(&conflict_case, 0, TEST_SHARED_VALUE, GROUP_OPERATION_ADD, 2);
    run_case(&conflict_case);
    ASSERT_TRUE(whole_group_flagged(&conflict_case, 0));
    ASSERT_EQ_INT(conflict_case.conflicts.count, 1);
}

static void test_find_board_conflicts_equal_numbers_are_in_order(void)
{
    ConflictCase conflict_case;
    start_case(&conflict_case, NULL, 0);
    set_group(&conflict_case, 0, 2, GROUP_OPERATION_NONE, 2);
    run_case(&conflict_case);
    ASSERT_TRUE(nothing_flagged(&conflict_case));
}

static void test_find_board_conflicts_out_of_range_number_is_flagged(void)
{
    ConflictCase conflict_case;
    start_case(&conflict_case, NULL, 0);
    set_group(&conflict_case, 0, TEST_SHARED_VALUE, GROUP_OPERATION_NONE,
              TEST_OUT_OF_RANGE_HIGH);
    set_group(&conflict_case, 1, TEST_OUT_OF_RANGE_LOW, GROUP_OPERATION_NONE,
              3);
    run_case(&conflict_case);
    ASSERT_TRUE(field_flagged(&conflict_case, 0, GROUP_FIELD_GREATER));
    ASSERT_FALSE(field_flagged(&conflict_case, 0, GROUP_FIELD_SMALLER));
    ASSERT_TRUE(field_flagged(&conflict_case, 1, GROUP_FIELD_SMALLER));
    ASSERT_FALSE(field_flagged(&conflict_case, 1, GROUP_FIELD_GREATER));
    ASSERT_EQ_INT(conflict_case.conflicts.count, 2);
    ASSERT_TRUE(result_is_well_formed(&conflict_case));
}

static void
test_find_board_conflicts_both_numbers_out_of_range_are_flagged(void)
{
    ConflictCase conflict_case;
    start_case(&conflict_case, NULL, 0);
    set_group(&conflict_case, 0, TEST_OUT_OF_RANGE_HIGH, GROUP_OPERATION_NONE,
              TEST_OUT_OF_RANGE_LOW);
    run_case(&conflict_case);
    ASSERT_TRUE(field_flagged(&conflict_case, 0, GROUP_FIELD_SMALLER));
    ASSERT_TRUE(field_flagged(&conflict_case, 0, GROUP_FIELD_GREATER));
    ASSERT_FALSE(field_flagged(&conflict_case, 0, GROUP_FIELD_OPERATION));
}

static void test_find_board_conflicts_range_boundaries_are_not_flagged(void)
{
    ConflictCase conflict_case;
    start_case(&conflict_case, NULL, 0);
    set_group(&conflict_case, 14, TETONOR_MIN_VALUE, GROUP_OPERATION_ADD,
              TETONOR_MAX_VALUE);
    run_case(&conflict_case);
    ASSERT_TRUE(nothing_flagged(&conflict_case));
}

static void test_find_board_conflicts_typed_slot_out_of_order_is_flagged(void)
{
    const int hidden[] = {TEST_TYPED_SLOT};
    ConflictCase conflict_case;
    start_case(&conflict_case, hidden, 1);
    conflict_case.strip[TEST_TYPED_SLOT] = TEST_BIG_TYPED_VALUE;
    run_case(&conflict_case);
    ASSERT_TRUE(conflict_case.conflicts.slots[TEST_TYPED_SLOT]);
    ASSERT_EQ_INT(flagged_box_count(&conflict_case.conflicts), 1);
    ASSERT_EQ_INT(conflict_case.conflicts.count, 1);
    ASSERT_TRUE(result_is_well_formed(&conflict_case));
}

static void
test_find_board_conflicts_typed_slot_below_a_neighbour_is_flagged(void)
{
    const int hidden[] = {TEST_TYPED_SLOT};
    ConflictCase conflict_case;
    start_case(&conflict_case, hidden, 1);
    conflict_case.strip[TEST_TYPED_SLOT] = 0;
    run_case(&conflict_case);
    ASSERT_TRUE(conflict_case.conflicts.slots[TEST_TYPED_SLOT]);
    ASSERT_EQ_INT(conflict_case.conflicts.count, 1);
}

static void test_find_board_conflicts_typed_slot_in_order_is_not_flagged(void)
{
    const int hidden[]   = {TEST_TYPED_SLOT};
    const int in_order[] = {4, 5, 6};
    ConflictCase conflict_case;
    start_case(&conflict_case, hidden, 1);
    for (size_t i = 0; i < sizeof in_order / sizeof in_order[0]; i++)
    {
        conflict_case.strip[TEST_TYPED_SLOT] = in_order[i];
        run_case(&conflict_case);
        ASSERT_TRUE(nothing_flagged(&conflict_case));
    }
}

static void test_find_board_conflicts_empty_slot_is_not_flagged(void)
{
    const int hidden[] = {TEST_TYPED_SLOT};
    ConflictCase conflict_case;
    start_case(&conflict_case, hidden, 1);
    run_case(&conflict_case);
    ASSERT_TRUE(nothing_flagged(&conflict_case));
}

static void
test_find_board_conflicts_two_typed_slots_out_of_order_flag_both(void)
{
    const int hidden[] = {TEST_TYPED_SLOT, TEST_NEXT_TYPED_SLOT};
    ConflictCase conflict_case;
    start_case(&conflict_case, hidden, 2);
    conflict_case.strip[TEST_TYPED_SLOT]      = 6;
    conflict_case.strip[TEST_NEXT_TYPED_SLOT] = 5;
    run_case(&conflict_case);
    ASSERT_TRUE(conflict_case.conflicts.slots[TEST_TYPED_SLOT]);
    ASSERT_TRUE(conflict_case.conflicts.slots[TEST_NEXT_TYPED_SLOT]);
    ASSERT_EQ_INT(flagged_box_count(&conflict_case.conflicts), 2);
    ASSERT_EQ_INT(conflict_case.conflicts.count, 2);
    ASSERT_TRUE(result_is_well_formed(&conflict_case));
}

static void
test_find_board_conflicts_two_typed_slots_in_order_are_not_flagged(void)
{
    const int hidden[] = {TEST_TYPED_SLOT, TEST_NEXT_TYPED_SLOT};
    ConflictCase conflict_case;
    start_case(&conflict_case, hidden, 2);
    conflict_case.strip[TEST_TYPED_SLOT]      = 5;
    conflict_case.strip[TEST_NEXT_TYPED_SLOT] = 5;
    run_case(&conflict_case);
    ASSERT_TRUE(nothing_flagged(&conflict_case));
}

static void test_find_board_conflicts_typed_slot_out_of_range_is_flagged(void)
{
    const int hidden[] = {TEST_TYPED_SLOT};
    ConflictCase conflict_case;
    start_case(&conflict_case, hidden, 1);
    conflict_case.strip[TEST_TYPED_SLOT] = TEST_OUT_OF_RANGE_HIGH;
    run_case(&conflict_case);
    ASSERT_TRUE(conflict_case.conflicts.slots[TEST_TYPED_SLOT]);
    ASSERT_EQ_INT(conflict_case.conflicts.count, 1);
    conflict_case.strip[TEST_TYPED_SLOT] = TEST_OUT_OF_RANGE_LOW;
    run_case(&conflict_case);
    ASSERT_TRUE(conflict_case.conflicts.slots[TEST_TYPED_SLOT]);
    ASSERT_EQ_INT(conflict_case.conflicts.count, 1);
}

static void test_find_board_conflicts_given_slot_is_never_flagged(void)
{
    const int hidden[] = {TEST_TYPED_SLOT, TEST_NEXT_TYPED_SLOT};
    ConflictCase conflict_case;
    start_case(&conflict_case, hidden, 2);
    conflict_case.strip[TEST_TYPED_SLOT]      = TEST_BIG_TYPED_VALUE;
    conflict_case.strip[TEST_NEXT_TYPED_SLOT] = 1;
    run_case(&conflict_case);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        bool editable = i == TEST_TYPED_SLOT || i == TEST_NEXT_TYPED_SLOT;
        ASSERT_EQ_INT(conflict_case.conflicts.slots[i], editable);
    }
}

static void
test_find_board_conflicts_add_overuse_flags_numbers_and_operation(void)
{
    ConflictCase conflict_case;
    start_case(&conflict_case, NULL, 0);
    set_group(&conflict_case, 0, TEST_SHARED_VALUE, GROUP_OPERATION_ADD,
              TETONOR_EMPTY_ENTRY);
    set_group(&conflict_case, 1, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_ADD,
              TEST_SHARED_VALUE);
    run_case(&conflict_case);
    ASSERT_TRUE(field_flagged(&conflict_case, 0, GROUP_FIELD_SMALLER));
    ASSERT_TRUE(field_flagged(&conflict_case, 0, GROUP_FIELD_OPERATION));
    ASSERT_FALSE(field_flagged(&conflict_case, 0, GROUP_FIELD_GREATER));
    ASSERT_FALSE(field_flagged(&conflict_case, 1, GROUP_FIELD_SMALLER));
    ASSERT_TRUE(field_flagged(&conflict_case, 1, GROUP_FIELD_OPERATION));
    ASSERT_TRUE(field_flagged(&conflict_case, 1, GROUP_FIELD_GREATER));
    ASSERT_EQ_INT(flagged_box_count(&conflict_case.conflicts), 4);
    ASSERT_EQ_INT(conflict_case.conflicts.count, 2);
    ASSERT_TRUE(result_is_well_formed(&conflict_case));
}

static void test_find_board_conflicts_multiply_overuse_is_flagged(void)
{
    ConflictCase conflict_case;
    start_case(&conflict_case, NULL, 0);
    set_group(&conflict_case, 2, TEST_SHARED_VALUE, GROUP_OPERATION_MULTIPLY,
              TETONOR_EMPTY_ENTRY);
    set_group(&conflict_case, 3, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_MULTIPLY,
              TEST_SHARED_VALUE);
    run_case(&conflict_case);
    ASSERT_TRUE(field_flagged(&conflict_case, 2, GROUP_FIELD_SMALLER));
    ASSERT_TRUE(field_flagged(&conflict_case, 2, GROUP_FIELD_OPERATION));
    ASSERT_FALSE(field_flagged(&conflict_case, 2, GROUP_FIELD_GREATER));
    ASSERT_FALSE(field_flagged(&conflict_case, 3, GROUP_FIELD_SMALLER));
    ASSERT_TRUE(field_flagged(&conflict_case, 3, GROUP_FIELD_OPERATION));
    ASSERT_TRUE(field_flagged(&conflict_case, 3, GROUP_FIELD_GREATER));
    ASSERT_EQ_INT(conflict_case.conflicts.count, 2);
    ASSERT_TRUE(result_is_well_formed(&conflict_case));
}

static void test_find_board_conflicts_overuse_without_operation_is_flagged(void)
{
    ConflictCase conflict_case;
    start_case(&conflict_case, NULL, 0);
    set_group(&conflict_case, 0, TEST_SHARED_VALUE, GROUP_OPERATION_NONE,
              TETONOR_EMPTY_ENTRY);
    set_group(&conflict_case, 1, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_NONE,
              TEST_SHARED_VALUE);
    set_group(&conflict_case, 2, TEST_SHARED_VALUE, GROUP_OPERATION_NONE,
              TETONOR_EMPTY_ENTRY);
    run_case(&conflict_case);
    ASSERT_TRUE(field_flagged(&conflict_case, 0, GROUP_FIELD_SMALLER));
    ASSERT_TRUE(field_flagged(&conflict_case, 1, GROUP_FIELD_GREATER));
    ASSERT_TRUE(field_flagged(&conflict_case, 2, GROUP_FIELD_SMALLER));
    ASSERT_EQ_INT(flagged_box_count(&conflict_case.conflicts), 3);
    ASSERT_EQ_INT(conflict_case.conflicts.count, 3);
    ASSERT_TRUE(result_is_well_formed(&conflict_case));
}

static void
test_find_board_conflicts_overuse_flags_number_of_other_operation(void)
{
    ConflictCase conflict_case;
    start_case(&conflict_case, NULL, 0);
    set_group(&conflict_case, 0, TEST_SHARED_VALUE, GROUP_OPERATION_ADD,
              TETONOR_EMPTY_ENTRY);
    set_group(&conflict_case, 1, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_ADD,
              TEST_SHARED_VALUE);
    set_group(&conflict_case, 2, TEST_SHARED_VALUE, GROUP_OPERATION_MULTIPLY,
              TETONOR_EMPTY_ENTRY);
    run_case(&conflict_case);
    ASSERT_TRUE(field_flagged(&conflict_case, 2, GROUP_FIELD_SMALLER));
    ASSERT_FALSE(field_flagged(&conflict_case, 2, GROUP_FIELD_OPERATION));
    ASSERT_TRUE(field_flagged(&conflict_case, 0, GROUP_FIELD_OPERATION));
    ASSERT_TRUE(field_flagged(&conflict_case, 1, GROUP_FIELD_OPERATION));
    ASSERT_EQ_INT(conflict_case.conflicts.count, 3);
    ASSERT_TRUE(result_is_well_formed(&conflict_case));
}

static void test_find_board_conflicts_uses_within_capacity_are_not_flagged(void)
{
    ConflictCase conflict_case;
    start_case(&conflict_case, NULL, 0);
    set_group(&conflict_case, 0, TEST_SHARED_VALUE, GROUP_OPERATION_ADD,
              TETONOR_EMPTY_ENTRY);
    run_case(&conflict_case);
    ASSERT_TRUE(nothing_flagged(&conflict_case));
    set_group(&conflict_case, 1, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_MULTIPLY,
              TEST_SHARED_VALUE);
    run_case(&conflict_case);
    ASSERT_TRUE(nothing_flagged(&conflict_case));
}

static void
test_find_board_conflicts_use_a_hidden_slot_can_hold_is_not_flagged(void)
{
    const int hidden[] = {TEST_WINDOW_SLOT};
    ConflictCase conflict_case;
    start_case(&conflict_case, hidden, 1);
    set_group(&conflict_case, 0, TEST_SHARED_VALUE, GROUP_OPERATION_ADD,
              TETONOR_EMPTY_ENTRY);
    set_group(&conflict_case, 1, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_ADD,
              TEST_SHARED_VALUE);
    run_case(&conflict_case);
    ASSERT_TRUE(nothing_flagged(&conflict_case));
    conflict_case.strip[TEST_WINDOW_SLOT] = 4;
    run_case(&conflict_case);
    ASSERT_TRUE(nothing_flagged(&conflict_case));
}

static void test_find_board_conflicts_hidden_slot_capacity_is_exact(void)
{
    const int hidden[] = {TEST_WINDOW_SLOT};
    ConflictCase conflict_case;
    start_case(&conflict_case, hidden, 1);
    set_group(&conflict_case, 0, TEST_SHARED_VALUE, GROUP_OPERATION_ADD,
              TETONOR_EMPTY_ENTRY);
    set_group(&conflict_case, 1, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_ADD,
              TEST_SHARED_VALUE);
    set_group(&conflict_case, 2, TEST_SHARED_VALUE, GROUP_OPERATION_ADD,
              TETONOR_EMPTY_ENTRY);
    run_case(&conflict_case);
    ASSERT_TRUE(field_flagged(&conflict_case, 0, GROUP_FIELD_SMALLER));
    ASSERT_TRUE(field_flagged(&conflict_case, 1, GROUP_FIELD_GREATER));
    ASSERT_TRUE(field_flagged(&conflict_case, 2, GROUP_FIELD_SMALLER));
    ASSERT_EQ_INT(conflict_case.conflicts.count, 3);
    ASSERT_TRUE(result_is_well_formed(&conflict_case));
}

static void
test_find_board_conflicts_hidden_slot_outside_its_window_adds_none(void)
{
    const int hidden[]  = {TEST_WINDOW_SLOT};
    const int far_value = 7; /* given once, at index 7; slot 4 holds 3..5 */
    ConflictCase conflict_case;
    start_case(&conflict_case, hidden, 1);
    set_group(&conflict_case, 0, far_value, GROUP_OPERATION_ADD,
              TETONOR_EMPTY_ENTRY);
    set_group(&conflict_case, 1, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_ADD,
              far_value);
    run_case(&conflict_case);
    ASSERT_TRUE(field_flagged(&conflict_case, 0, GROUP_FIELD_SMALLER));
    ASSERT_TRUE(field_flagged(&conflict_case, 1, GROUP_FIELD_GREATER));
    ASSERT_EQ_INT(conflict_case.conflicts.count, 2);
}

static void
test_find_board_conflicts_pair_twice_with_same_operation_is_legal(void)
{
    ConflictCase conflict_case;
    start_pair_case(&conflict_case, 2, 2);
    set_group(&conflict_case, 0, TEST_PAIR_SMALLER, GROUP_OPERATION_ADD,
              TEST_PAIR_GREATER);
    set_group(&conflict_case, 1, TEST_PAIR_SMALLER, GROUP_OPERATION_ADD,
              TEST_PAIR_GREATER);
    run_case(&conflict_case);
    ASSERT_TRUE(nothing_flagged(&conflict_case));
}

static void
test_find_board_conflicts_pair_exceeding_partner_cells_is_flagged(void)
{
    ConflictCase conflict_case;
    start_pair_case(&conflict_case, 2, 1);
    set_group(&conflict_case, 0, TEST_PAIR_SMALLER, GROUP_OPERATION_ADD,
              TEST_PAIR_GREATER);
    set_group(&conflict_case, 1, TEST_PAIR_SMALLER, GROUP_OPERATION_ADD,
              TEST_PAIR_GREATER);
    run_case(&conflict_case);
    ASSERT_TRUE(whole_group_flagged(&conflict_case, 0));
    ASSERT_TRUE(whole_group_flagged(&conflict_case, 1));
    ASSERT_EQ_INT(conflict_case.conflicts.count, 2);
    ASSERT_TRUE(result_is_well_formed(&conflict_case));
}

static void
test_find_board_conflicts_pair_within_partner_cells_is_not_flagged(void)
{
    ConflictCase conflict_case;
    start_pair_case(&conflict_case, 2, 1);
    set_group(&conflict_case, 0, TEST_PAIR_SMALLER, GROUP_OPERATION_ADD,
              TEST_PAIR_GREATER);
    run_case(&conflict_case);
    ASSERT_TRUE(nothing_flagged(&conflict_case));
}

static void
test_find_board_conflicts_multiply_pair_exceeding_sum_cells_flagged(void)
{
    ConflictCase conflict_case;
    start_pair_case(&conflict_case, 1, 2);
    set_group(&conflict_case, 1, TEST_PAIR_SMALLER, GROUP_OPERATION_MULTIPLY,
              TEST_PAIR_GREATER);
    set_group(&conflict_case, 2, TEST_PAIR_SMALLER, GROUP_OPERATION_MULTIPLY,
              TEST_PAIR_GREATER);
    run_case(&conflict_case);
    ASSERT_TRUE(whole_group_flagged(&conflict_case, 1));
    ASSERT_TRUE(whole_group_flagged(&conflict_case, 2));
    ASSERT_EQ_INT(conflict_case.conflicts.count, 2);
    ASSERT_TRUE(result_is_well_formed(&conflict_case));
}

static void
test_find_board_conflicts_multiply_pair_within_sum_cells_is_legal(void)
{
    ConflictCase conflict_case;
    start_pair_case(&conflict_case, 2, 2);
    set_group(&conflict_case, 2, TEST_PAIR_SMALLER, GROUP_OPERATION_MULTIPLY,
              TEST_PAIR_GREATER);
    set_group(&conflict_case, 3, TEST_PAIR_SMALLER, GROUP_OPERATION_MULTIPLY,
              TEST_PAIR_GREATER);
    run_case(&conflict_case);
    ASSERT_TRUE(nothing_flagged(&conflict_case));
}

static void test_find_board_conflicts_empty_boxes_are_never_flagged(void)
{
    const int hidden[] = {2, TEST_TYPED_SLOT, TEST_NEXT_TYPED_SLOT,
                          TEST_FAR_SLOT};
    ConflictCase conflict_case;
    start_case(&conflict_case, hidden, sizeof hidden / sizeof hidden[0]);
    conflict_case.strip[TEST_TYPED_SLOT] = TEST_BIG_TYPED_VALUE;
    set_group(&conflict_case, 0, TEST_SHARED_VALUE, GROUP_OPERATION_NONE, 2);
    set_group(&conflict_case, 1, TEST_SHARED_VALUE, GROUP_OPERATION_ADD,
              TETONOR_EMPTY_ENTRY);
    set_group(&conflict_case, 2, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_ADD,
              TEST_SHARED_VALUE);
    set_group(&conflict_case, 3, TEST_SHARED_VALUE, GROUP_OPERATION_NONE,
              TETONOR_EMPTY_ENTRY);
    run_case(&conflict_case);
    ASSERT_TRUE(conflict_case.conflicts.count > 0);
    ASSERT_FALSE(conflict_case.conflicts.slots[2]);
    ASSERT_FALSE(conflict_case.conflicts.slots[TEST_NEXT_TYPED_SLOT]);
    ASSERT_FALSE(conflict_case.conflicts.slots[TEST_FAR_SLOT]);
    ASSERT_TRUE(empty_boxes_unflagged(&conflict_case));
    ASSERT_FALSE(field_flagged(&conflict_case, 0, GROUP_FIELD_OPERATION));
    ASSERT_FALSE(field_flagged(&conflict_case, 1, GROUP_FIELD_GREATER));
    ASSERT_FALSE(field_flagged(&conflict_case, 2, GROUP_FIELD_SMALLER));
    ASSERT_FALSE(field_flagged(&conflict_case, 3, GROUP_FIELD_OPERATION));
    ASSERT_FALSE(field_flagged(&conflict_case, 3, GROUP_FIELD_GREATER));
}

static void test_find_board_conflicts_count_counts_marked_items(void)
{
    const int hidden[] = {TEST_TYPED_SLOT};
    ConflictCase conflict_case;
    start_case(&conflict_case, hidden, 1);
    conflict_case.strip[TEST_TYPED_SLOT] = TEST_BIG_TYPED_VALUE;
    set_group(&conflict_case, 0, TEST_SHARED_VALUE, GROUP_OPERATION_NONE, 2);
    run_case(&conflict_case);
    ASSERT_EQ_INT(flagged_box_count(&conflict_case.conflicts), 3);
    ASSERT_EQ_INT(conflict_case.conflicts.count, 2);
}

static void test_find_board_conflicts_scrambled_boards_stay_well_formed(void)
{
    unsigned int state = 1;
    ConflictCase conflict_case;
    for (int round = 0; round < TEST_SCRAMBLE_ROUNDS; round++)
    {
        start_case(&conflict_case, scramble_hidden,
                   sizeof scramble_hidden / sizeof scramble_hidden[0]);
        for (size_t i = 0;
             i < sizeof scramble_hidden / sizeof scramble_hidden[0]; i++)
        {
            conflict_case.strip[scramble_hidden[i]] =
                random_scramble_value(&state);
        }
        for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
        {
            set_group(&conflict_case, cell, random_scramble_value(&state),
                      scramble_operations[next_random(&state) %
                                          (sizeof scramble_operations /
                                           sizeof scramble_operations[0])],
                      random_scramble_value(&state));
        }
        run_case(&conflict_case);
        ASSERT_TRUE(result_is_well_formed(&conflict_case));
        if (conflict_case.conflicts.count > 0)
        {
            ASSERT_FALSE(is_answer_correct(&conflict_case.puzzle,
                                           conflict_case.strip,
                                           conflict_case.groups));
        }
    }
}

static void test_find_board_conflicts_is_pure(void)
{
    const int hidden[] = {TEST_TYPED_SLOT, TEST_NEXT_TYPED_SLOT};
    ConflictCase conflict_case;
    start_case(&conflict_case, hidden, 2);
    conflict_case.strip[TEST_TYPED_SLOT]      = 6;
    conflict_case.strip[TEST_NEXT_TYPED_SLOT] = 5;
    set_group(&conflict_case, 0, 1, GROUP_OPERATION_ADD, 3);
    NumberGeneration puzzle_before = conflict_case.puzzle;
    int strip_before[TETONOR_CELL_COUNT];
    OperandGroup groups_before[TETONOR_CELL_COUNT];
    memcpy(strip_before, conflict_case.strip, sizeof strip_before);
    memcpy(groups_before, conflict_case.groups, sizeof groups_before);
    run_case(&conflict_case);
    BoardConflicts first = conflict_case.conflicts;
    ASSERT_TRUE(first.count > 0);
    ASSERT_TRUE(memcmp(&puzzle_before, &conflict_case.puzzle,
                       sizeof puzzle_before) == 0);
    ASSERT_TRUE(
        memcmp(strip_before, conflict_case.strip, sizeof strip_before) == 0);
    ASSERT_TRUE(
        memcmp(groups_before, conflict_case.groups, sizeof groups_before) == 0);
    run_case(&conflict_case);
    ASSERT_TRUE(conflicts_equal(&first, &conflict_case.conflicts));
}

static void test_find_board_conflicts_overwrites_the_whole_output(void)
{
    ConflictCase conflict_case;
    start_case(&conflict_case, NULL, 0);
    memcpy(conflict_case.strip, fixture_strip, sizeof conflict_case.strip);
    memcpy(conflict_case.groups, fixture_groups, sizeof conflict_case.groups);
    poison_conflicts(&conflict_case.conflicts);
    find_board_conflicts(&conflict_case.puzzle, conflict_case.strip,
                         conflict_case.groups, &conflict_case.conflicts);
    ASSERT_TRUE(nothing_flagged(&conflict_case));
}

static void test_group_has_conflict_is_true_when_any_box_is_flagged(void)
{
    for (int field = 0; field < TEST_FIELD_COUNT; field++)
    {
        BoardConflicts conflicts;
        memset(&conflicts, 0, sizeof conflicts);
        ASSERT_FALSE(group_has_conflict(&conflicts, 3));
        conflicts.fields[3][field] = true;
        ASSERT_TRUE(group_has_conflict(&conflicts, 3));
        ASSERT_FALSE(group_has_conflict(&conflicts, 2));
        ASSERT_FALSE(group_has_conflict(&conflicts, 4));
    }
}

static void test_group_has_conflict_covers_the_first_and_last_cell(void)
{
    BoardConflicts conflicts;
    memset(&conflicts, 0, sizeof conflicts);
    conflicts.fields[0][GROUP_FIELD_SMALLER]                      = true;
    conflicts.fields[TETONOR_CELL_COUNT - 1][GROUP_FIELD_GREATER] = true;
    ASSERT_TRUE(group_has_conflict(&conflicts, 0));
    ASSERT_TRUE(group_has_conflict(&conflicts, TETONOR_CELL_COUNT - 1));
}

static void test_group_has_conflict_ignores_flagged_slots(void)
{
    BoardConflicts conflicts;
    memset(&conflicts, 0, sizeof conflicts);
    conflicts.slots[3] = true;
    ASSERT_FALSE(group_has_conflict(&conflicts, 3));
}

static void test_group_has_conflict_rejects_cells_out_of_range(void)
{
    BoardConflicts conflicts;
    memset(&conflicts, 0, sizeof conflicts);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        conflicts.fields[i][GROUP_FIELD_OPERATION] = true;
    }
    ASSERT_FALSE(group_has_conflict(&conflicts, -1));
    ASSERT_FALSE(group_has_conflict(&conflicts, TETONOR_CELL_COUNT));
}

int main(void)
{
    RUN_TEST(test_conflict_constants);
    RUN_TEST(test_find_board_conflicts_solved_fixture_has_none);
    RUN_TEST(
        test_find_board_conflicts_solved_fixture_with_typed_slots_has_none);
    RUN_TEST(test_find_board_conflicts_generated_solutions_have_none);
    RUN_TEST(test_find_board_conflicts_partial_solutions_have_none);
    RUN_TEST(test_find_board_conflicts_empty_board_has_none);
    RUN_TEST(test_find_board_conflicts_partial_group_is_not_checked_for_target);
    RUN_TEST(test_find_board_conflicts_target_mismatch_flags_whole_group);
    RUN_TEST(test_find_board_conflicts_matching_group_is_not_flagged);
    RUN_TEST(test_find_board_conflicts_wrong_operation_is_flagged);
    RUN_TEST(test_find_board_conflicts_order_flags_both_numbers);
    RUN_TEST(test_find_board_conflicts_order_with_operation_flags_whole_group);
    RUN_TEST(test_find_board_conflicts_equal_numbers_are_in_order);
    RUN_TEST(test_find_board_conflicts_out_of_range_number_is_flagged);
    RUN_TEST(test_find_board_conflicts_both_numbers_out_of_range_are_flagged);
    RUN_TEST(test_find_board_conflicts_range_boundaries_are_not_flagged);
    RUN_TEST(test_find_board_conflicts_typed_slot_out_of_order_is_flagged);
    RUN_TEST(test_find_board_conflicts_typed_slot_below_a_neighbour_is_flagged);
    RUN_TEST(test_find_board_conflicts_typed_slot_in_order_is_not_flagged);
    RUN_TEST(test_find_board_conflicts_empty_slot_is_not_flagged);
    RUN_TEST(test_find_board_conflicts_two_typed_slots_out_of_order_flag_both);
    RUN_TEST(
        test_find_board_conflicts_two_typed_slots_in_order_are_not_flagged);
    RUN_TEST(test_find_board_conflicts_typed_slot_out_of_range_is_flagged);
    RUN_TEST(test_find_board_conflicts_given_slot_is_never_flagged);
    RUN_TEST(test_find_board_conflicts_add_overuse_flags_numbers_and_operation);
    RUN_TEST(test_find_board_conflicts_multiply_overuse_is_flagged);
    RUN_TEST(test_find_board_conflicts_overuse_without_operation_is_flagged);
    RUN_TEST(test_find_board_conflicts_overuse_flags_number_of_other_operation);
    RUN_TEST(test_find_board_conflicts_uses_within_capacity_are_not_flagged);
    RUN_TEST(
        test_find_board_conflicts_use_a_hidden_slot_can_hold_is_not_flagged);
    RUN_TEST(test_find_board_conflicts_hidden_slot_capacity_is_exact);
    RUN_TEST(
        test_find_board_conflicts_hidden_slot_outside_its_window_adds_none);
    RUN_TEST(test_find_board_conflicts_pair_twice_with_same_operation_is_legal);
    RUN_TEST(test_find_board_conflicts_pair_exceeding_partner_cells_is_flagged);
    RUN_TEST(
        test_find_board_conflicts_pair_within_partner_cells_is_not_flagged);
    RUN_TEST(
        test_find_board_conflicts_multiply_pair_exceeding_sum_cells_flagged);
    RUN_TEST(test_find_board_conflicts_multiply_pair_within_sum_cells_is_legal);
    RUN_TEST(test_find_board_conflicts_empty_boxes_are_never_flagged);
    RUN_TEST(test_find_board_conflicts_count_counts_marked_items);
    RUN_TEST(test_find_board_conflicts_scrambled_boards_stay_well_formed);
    RUN_TEST(test_find_board_conflicts_is_pure);
    RUN_TEST(test_find_board_conflicts_overwrites_the_whole_output);
    RUN_TEST(test_group_has_conflict_is_true_when_any_box_is_flagged);
    RUN_TEST(test_group_has_conflict_covers_the_first_and_last_cell);
    RUN_TEST(test_group_has_conflict_ignores_flagged_slots);
    RUN_TEST(test_group_has_conflict_rejects_cells_out_of_range);
    return test_harness_result();
}
