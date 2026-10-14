#include <stdbool.h>
#include <string.h>

#include "../test_harness.h"
#include "fixture_puzzle.h"
#include "../../src/game/slot_usage.h"

#define TEST_STRIP_BASE 10 /* the strip holds 10..25: all distinct */
#define TEST_VALUE 5 /* outside the strip, so only placed copies hold it */
#define TEST_HIGH_PARTNER 28 /* partner of TEST_VALUE as the smaller number */
#define TEST_LOW_PARTNER 1   /* partner of TEST_VALUE as the greater number */
#define TEST_FIRST_SLOT 3
#define TEST_SECOND_SLOT 9
#define TEST_THIRD_SLOT 12
#define TEST_BELOW_RANGE (-5)
#define TEST_ABOVE_RANGE 30
#define TEST_FIXTURE_DUPLICATE_SLOT 2
#define TEST_HIDDEN_COUNT 4
#define TEST_FIXTURE_ORIGINAL_SLOT 1
#define TEST_FIXTURE_ORIGINAL_VALUE 1

static const int fixture_hidden[TEST_HIDDEN_COUNT] = {2, 5, 6, 15};
static const int box_slots[] = {TEST_FIRST_SLOT, TEST_SECOND_SLOT,
                                TEST_THIRD_SLOT};

typedef struct
{
    int strip[TETONOR_CELL_COUNT];
    OperandGroup groups[TETONOR_CELL_COUNT];
    SlotUsage usages[TETONOR_CELL_COUNT];
} UsageCase;

/* Starts a case whose strip holds TEST_VALUE in the first box_count slots of
 * box_slots and distinct values elsewhere, with every group empty. The output
 * is pre-filled with a non-NONE value to show that it is fully overwritten. */
static void start_case(UsageCase* usage_case, int box_count)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        usage_case->strip[i] = TEST_STRIP_BASE + i;
        clear_operand_group(&usage_case->groups[i]);
        usage_case->usages[i] = SLOT_USAGE_TWICE;
    }
    for (int i = 0; i < box_count; i++)
    {
        usage_case->strip[box_slots[i]] = TEST_VALUE;
    }
}

static void run_case(UsageCase* usage_case)
{
    compute_slot_usages(usage_case->strip, usage_case->groups,
                        usage_case->usages);
}

static void set_group(UsageCase* usage_case, int cell, int smaller,
                      GroupOperation operation, int greater)
{
    OperandGroup* group = &usage_case->groups[cell];
    group->smaller      = smaller;
    group->operation    = operation;
    group->greater      = greater;
}

/* One use of TEST_VALUE in the given cell, alternating between the smaller and
 * the greater box so that both are exercised. */
static void set_use(UsageCase* usage_case, int cell, GroupOperation operation)
{
    if (cell % 2 == 0)
    {
        set_group(usage_case, cell, TEST_VALUE, operation, TEST_HIGH_PARTNER);
    }
    else
    {
        set_group(usage_case, cell, TEST_LOW_PARTNER, operation, TEST_VALUE);
    }
}

static void set_uses(UsageCase* usage_case, int adds, int multiplies)
{
    int cell = 0;
    for (int i = 0; i < adds; i++)
    {
        set_use(usage_case, cell++, GROUP_OPERATION_ADD);
    }
    for (int i = 0; i < multiplies; i++)
    {
        set_use(usage_case, cell++, GROUP_OPERATION_MULTIPLY);
    }
}

static int marked_count(const UsageCase* usage_case)
{
    int count = 0;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (usage_case->usages[i] != SLOT_USAGE_NONE)
        {
            count++;
        }
    }
    return count;
}

static void test_compute_slot_usages_without_groups_marks_nothing(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 0);
    run_case(&usage_case);
    ASSERT_EQ_INT(marked_count(&usage_case), 0);
}

static void test_compute_slot_usages_single_add_use_is_once(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_group(&usage_case, 0, TEST_VALUE, GROUP_OPERATION_ADD,
              TEST_HIGH_PARTNER);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(marked_count(&usage_case), 1);
}

static void test_compute_slot_usages_single_multiply_use_is_once(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_group(&usage_case, 0, TEST_LOW_PARTNER, GROUP_OPERATION_MULTIPLY,
              TEST_VALUE);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(marked_count(&usage_case), 1);
}

static void test_compute_slot_usages_use_in_greater_box_counts(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_group(&usage_case, 0, TEST_LOW_PARTNER, GROUP_OPERATION_ADD,
              TEST_VALUE);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
}

static void test_compute_slot_usages_use_in_smaller_box_counts(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_group(&usage_case, 0, TEST_VALUE, GROUP_OPERATION_MULTIPLY,
              TEST_HIGH_PARTNER);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
}

static void test_compute_slot_usages_use_in_any_cell_counts(void)
{
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        UsageCase usage_case;
        start_case(&usage_case, 1);
        set_group(&usage_case, cell, TEST_VALUE, GROUP_OPERATION_ADD,
                  TEST_HIGH_PARTNER);
        run_case(&usage_case);
        ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
    }
}

static void test_compute_slot_usages_two_adds_one_box_is_once(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_uses(&usage_case, 2, 0);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(marked_count(&usage_case), 1);
}

static void test_compute_slot_usages_two_multiplies_one_box_is_once(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_uses(&usage_case, 0, 2);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(marked_count(&usage_case), 1);
}

static void test_compute_slot_usages_one_add_and_one_multiply_is_twice(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_uses(&usage_case, 1, 1);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_TWICE);
    ASSERT_EQ_INT(marked_count(&usage_case), 1);
}

static void test_compute_slot_usages_third_use_keeps_twice(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_uses(&usage_case, 2, 1);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_TWICE);
    start_case(&usage_case, 1);
    set_uses(&usage_case, 1, 2);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_TWICE);
}

static void test_compute_slot_usages_many_uses_keep_twice(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_uses(&usage_case, 3, 3);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_TWICE);
    ASSERT_EQ_INT(marked_count(&usage_case), 1);
}

static void
test_compute_slot_usages_equal_operands_count_two_uses_of_one_operation(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_group(&usage_case, 0, TEST_VALUE, GROUP_OPERATION_ADD, TEST_VALUE);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(marked_count(&usage_case), 1);
}

static void
test_compute_slot_usages_equal_operands_in_both_operations_are_twice(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_group(&usage_case, 0, TEST_VALUE, GROUP_OPERATION_ADD, TEST_VALUE);
    set_group(&usage_case, 1, TEST_VALUE, GROUP_OPERATION_MULTIPLY, TEST_VALUE);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_TWICE);
}

static void
test_compute_slot_usages_equal_operands_and_a_multiply_use_are_twice(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_group(&usage_case, 0, TEST_VALUE, GROUP_OPERATION_ADD, TEST_VALUE);
    set_group(&usage_case, 1, TEST_VALUE, GROUP_OPERATION_MULTIPLY,
              TEST_HIGH_PARTNER);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_TWICE);
}

static void test_compute_slot_usages_two_boxes_two_adds_are_both_once(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 2);
    set_uses(&usage_case, 2, 0);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(usage_case.usages[TEST_SECOND_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(marked_count(&usage_case), 2);
}

static void
test_compute_slot_usages_two_boxes_two_multiplies_are_both_once(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 2);
    set_uses(&usage_case, 0, 2);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(usage_case.usages[TEST_SECOND_SLOT], SLOT_USAGE_ONCE);
}

static void test_compute_slot_usages_two_boxes_one_add_one_multiply(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 2);
    set_uses(&usage_case, 1, 1);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_TWICE);
    ASSERT_EQ_INT(usage_case.usages[TEST_SECOND_SLOT], SLOT_USAGE_NONE);
    ASSERT_EQ_INT(marked_count(&usage_case), 1);
}

static void test_compute_slot_usages_two_boxes_two_adds_one_multiply(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 2);
    set_uses(&usage_case, 2, 1);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_TWICE);
    ASSERT_EQ_INT(usage_case.usages[TEST_SECOND_SLOT], SLOT_USAGE_ONCE);
}

static void test_compute_slot_usages_two_boxes_two_adds_two_multiplies(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 2);
    set_uses(&usage_case, 2, 2);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_TWICE);
    ASSERT_EQ_INT(usage_case.usages[TEST_SECOND_SLOT], SLOT_USAGE_TWICE);
}

static void test_compute_slot_usages_two_boxes_three_adds_one_multiply(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 2);
    set_uses(&usage_case, 3, 1);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_TWICE);
    ASSERT_EQ_INT(usage_case.usages[TEST_SECOND_SLOT], SLOT_USAGE_ONCE);
}

static void test_compute_slot_usages_two_boxes_one_multiply_only(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 2);
    set_uses(&usage_case, 0, 1);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(usage_case.usages[TEST_SECOND_SLOT], SLOT_USAGE_NONE);
    ASSERT_EQ_INT(marked_count(&usage_case), 1);
}

static void test_compute_slot_usages_two_boxes_one_add_only(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 2);
    set_uses(&usage_case, 1, 0);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(usage_case.usages[TEST_SECOND_SLOT], SLOT_USAGE_NONE);
}

static void test_compute_slot_usages_two_boxes_without_uses_are_unmarked(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 2);
    run_case(&usage_case);
    ASSERT_EQ_INT(marked_count(&usage_case), 0);
}

static void test_compute_slot_usages_two_boxes_many_uses_are_both_twice(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 2);
    set_uses(&usage_case, 4, 4);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_TWICE);
    ASSERT_EQ_INT(usage_case.usages[TEST_SECOND_SLOT], SLOT_USAGE_TWICE);
}

static void test_compute_slot_usages_three_boxes_follow_strip_order(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 3);
    set_uses(&usage_case, 2, 1);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_TWICE);
    ASSERT_EQ_INT(usage_case.usages[TEST_SECOND_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(usage_case.usages[TEST_THIRD_SLOT], SLOT_USAGE_NONE);
    start_case(&usage_case, 3);
    set_uses(&usage_case, 1, 1);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_TWICE);
    ASSERT_EQ_INT(usage_case.usages[TEST_SECOND_SLOT], SLOT_USAGE_NONE);
    ASSERT_EQ_INT(usage_case.usages[TEST_THIRD_SLOT], SLOT_USAGE_NONE);
}

static void
test_compute_slot_usages_first_of_equal_slots_takes_the_single_use(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 0);
    usage_case.strip[0]                      = TEST_VALUE;
    usage_case.strip[TETONOR_CELL_COUNT - 1] = TEST_VALUE;
    set_uses(&usage_case, 1, 0);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[0], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(usage_case.usages[TETONOR_CELL_COUNT - 1], SLOT_USAGE_NONE);
    ASSERT_EQ_INT(marked_count(&usage_case), 1);
}

static void test_compute_slot_usages_operation_none_contributes_nothing(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_group(&usage_case, 0, TEST_VALUE, GROUP_OPERATION_NONE, TEST_VALUE);
    run_case(&usage_case);
    ASSERT_EQ_INT(marked_count(&usage_case), 0);
}

static void
test_compute_slot_usages_operation_none_does_not_hide_other_uses(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_group(&usage_case, 0, TEST_VALUE, GROUP_OPERATION_NONE, TEST_VALUE);
    set_group(&usage_case, 1, TEST_VALUE, GROUP_OPERATION_ADD,
              TEST_HIGH_PARTNER);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
}

static void
test_compute_slot_usages_empty_smaller_box_counts_only_the_greater(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_group(&usage_case, 0, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_ADD,
              TEST_VALUE);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(marked_count(&usage_case), 1);
}

static void
test_compute_slot_usages_empty_greater_box_counts_only_the_smaller(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_group(&usage_case, 0, TEST_VALUE, GROUP_OPERATION_MULTIPLY,
              TETONOR_EMPTY_ENTRY);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(marked_count(&usage_case), 1);
}

static void
test_compute_slot_usages_group_with_both_boxes_empty_marks_nothing(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_group(&usage_case, 0, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_ADD,
              TETONOR_EMPTY_ENTRY);
    set_group(&usage_case, 1, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_MULTIPLY,
              TETONOR_EMPTY_ENTRY);
    run_case(&usage_case);
    ASSERT_EQ_INT(marked_count(&usage_case), 0);
}

static void test_compute_slot_usages_invalid_group_still_counts(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_group(&usage_case, 0, TEST_HIGH_PARTNER, GROUP_OPERATION_ADD,
              TEST_VALUE);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
}

static void test_compute_slot_usages_empty_slot_is_never_marked(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 0);
    usage_case.strip[0] = TETONOR_EMPTY_ENTRY;
    for (int cell = 0; cell < TETONOR_CELL_COUNT; cell++)
    {
        int value = TEST_STRIP_BASE + cell;
        set_group(&usage_case, cell, value, GROUP_OPERATION_ADD, value);
    }
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[0], SLOT_USAGE_NONE);
    for (int i = 1; i < TETONOR_CELL_COUNT; i++)
    {
        ASSERT_EQ_INT(usage_case.usages[i], SLOT_USAGE_ONCE);
    }
}

static void test_compute_slot_usages_empty_slots_do_not_match_each_other(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 0);
    usage_case.strip[0] = TETONOR_EMPTY_ENTRY;
    usage_case.strip[1] = TETONOR_EMPTY_ENTRY;
    set_group(&usage_case, 0, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_ADD,
              TETONOR_EMPTY_ENTRY);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[0], SLOT_USAGE_NONE);
    ASSERT_EQ_INT(usage_case.usages[1], SLOT_USAGE_NONE);
    ASSERT_EQ_INT(marked_count(&usage_case), 0);
}

static void test_compute_slot_usages_all_slots_empty_marks_nothing(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 0);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        usage_case.strip[i] = TETONOR_EMPTY_ENTRY;
    }
    set_uses(&usage_case, 2, 2);
    run_case(&usage_case);
    ASSERT_EQ_INT(marked_count(&usage_case), 0);
}

static void test_compute_slot_usages_empty_slot_takes_no_rank(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 0);
    usage_case.strip[0]               = TETONOR_EMPTY_ENTRY;
    usage_case.strip[TEST_FIRST_SLOT] = TEST_VALUE;
    set_uses(&usage_case, 1, 0);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
}

static void test_compute_slot_usages_empty_slot_between_equal_values(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 0);
    usage_case.strip[TEST_FIRST_SLOT]     = TEST_VALUE;
    usage_case.strip[TEST_FIRST_SLOT + 1] = TETONOR_EMPTY_ENTRY;
    usage_case.strip[TEST_SECOND_SLOT]    = TEST_VALUE;
    set_uses(&usage_case, 2, 0);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT + 1], SLOT_USAGE_NONE);
    ASSERT_EQ_INT(usage_case.usages[TEST_SECOND_SLOT], SLOT_USAGE_ONCE);
}

static void test_compute_slot_usages_range_boundaries_count(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 0);
    usage_case.strip[0]                      = TETONOR_MIN_VALUE;
    usage_case.strip[TETONOR_CELL_COUNT - 1] = TETONOR_MAX_VALUE;
    set_group(&usage_case, 0, TETONOR_MIN_VALUE, GROUP_OPERATION_ADD,
              TETONOR_MAX_VALUE);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[0], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(usage_case.usages[TETONOR_CELL_COUNT - 1], SLOT_USAGE_ONCE);
    ASSERT_EQ_INT(marked_count(&usage_case), 2);
}

static void test_compute_slot_usages_out_of_range_operands_are_ignored(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_group(&usage_case, 0, TEST_BELOW_RANGE, GROUP_OPERATION_ADD,
              TEST_ABOVE_RANGE);
    set_group(&usage_case, 1, TETONOR_MIN_VALUE - 1, GROUP_OPERATION_MULTIPLY,
              TETONOR_MAX_VALUE + 1);
    run_case(&usage_case);
    ASSERT_EQ_INT(marked_count(&usage_case), 0);
}

static void test_compute_slot_usages_out_of_range_partner_keeps_the_use(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 1);
    set_group(&usage_case, 0, TEST_VALUE, GROUP_OPERATION_ADD,
              TEST_ABOVE_RANGE);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_ONCE);
}

static void test_compute_slot_usages_out_of_range_strip_value_is_unmarked(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 0);
    usage_case.strip[0]               = TEST_ABOVE_RANGE;
    usage_case.strip[1]               = TEST_BELOW_RANGE;
    usage_case.strip[TEST_FIRST_SLOT] = TETONOR_MAX_VALUE + 1;
    set_group(&usage_case, 0, TEST_ABOVE_RANGE, GROUP_OPERATION_ADD,
              TEST_BELOW_RANGE);
    run_case(&usage_case);
    ASSERT_EQ_INT(usage_case.usages[0], SLOT_USAGE_NONE);
    ASSERT_EQ_INT(usage_case.usages[1], SLOT_USAGE_NONE);
    ASSERT_EQ_INT(usage_case.usages[TEST_FIRST_SLOT], SLOT_USAGE_NONE);
}

static void test_compute_slot_usages_does_not_modify_its_inputs(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 2);
    set_uses(&usage_case, 2, 1);
    int strip_before[TETONOR_CELL_COUNT];
    OperandGroup groups_before[TETONOR_CELL_COUNT];
    memcpy(strip_before, usage_case.strip, sizeof strip_before);
    memcpy(groups_before, usage_case.groups, sizeof groups_before);
    run_case(&usage_case);
    ASSERT_TRUE(memcmp(strip_before, usage_case.strip, sizeof strip_before) ==
                0);
    ASSERT_TRUE(
        memcmp(groups_before, usage_case.groups, sizeof groups_before) == 0);
}

static void test_compute_slot_usages_is_repeatable(void)
{
    UsageCase usage_case;
    start_case(&usage_case, 2);
    set_uses(&usage_case, 2, 1);
    run_case(&usage_case);
    SlotUsage first[TETONOR_CELL_COUNT];
    memcpy(first, usage_case.usages, sizeof first);
    run_case(&usage_case);
    ASSERT_TRUE(memcmp(first, usage_case.usages, sizeof first) == 0);
}

static void test_compute_slot_usages_fixture_marks_every_slot_twice(void)
{
    SlotUsage usages[TETONOR_CELL_COUNT];
    compute_slot_usages(fixture_strip, fixture_groups, usages);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        ASSERT_EQ_INT(usages[i], SLOT_USAGE_TWICE);
    }
}

static void test_compute_slot_usages_hidden_fixture_slots_are_unmarked(void)
{
    NumberGeneration puzzle;
    build_fixture_puzzle(&puzzle, fixture_hidden, TEST_HIDDEN_COUNT);
    SlotUsage usages[TETONOR_CELL_COUNT];
    compute_slot_usages(puzzle.available_array, fixture_groups, usages);
    int hidden_seen = 0;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        bool hidden = puzzle.available_array[i] == TETONOR_HIDDEN_NUMBER;
        hidden_seen += hidden ? 1 : 0;
        ASSERT_EQ_INT(usages[i], hidden ? SLOT_USAGE_NONE : SLOT_USAGE_TWICE);
    }
    ASSERT_EQ_INT(hidden_seen, TEST_HIDDEN_COUNT);
}

static void test_compute_slot_usages_fixture_duplicate_is_unmarked(void)
{
    int strip[TETONOR_CELL_COUNT];
    memcpy(strip, fixture_strip, sizeof strip);
    strip[TEST_FIXTURE_DUPLICATE_SLOT] = TEST_FIXTURE_ORIGINAL_VALUE;
    SlotUsage usages[TETONOR_CELL_COUNT];
    compute_slot_usages(strip, fixture_groups, usages);
    ASSERT_EQ_INT(usages[TEST_FIXTURE_ORIGINAL_SLOT], SLOT_USAGE_TWICE);
    ASSERT_EQ_INT(usages[TEST_FIXTURE_DUPLICATE_SLOT], SLOT_USAGE_NONE);
}

int main(void)
{
    RUN_TEST(test_compute_slot_usages_without_groups_marks_nothing);
    RUN_TEST(test_compute_slot_usages_single_add_use_is_once);
    RUN_TEST(test_compute_slot_usages_single_multiply_use_is_once);
    RUN_TEST(test_compute_slot_usages_use_in_greater_box_counts);
    RUN_TEST(test_compute_slot_usages_use_in_smaller_box_counts);
    RUN_TEST(test_compute_slot_usages_use_in_any_cell_counts);
    RUN_TEST(test_compute_slot_usages_two_adds_one_box_is_once);
    RUN_TEST(test_compute_slot_usages_two_multiplies_one_box_is_once);
    RUN_TEST(test_compute_slot_usages_one_add_and_one_multiply_is_twice);
    RUN_TEST(test_compute_slot_usages_third_use_keeps_twice);
    RUN_TEST(test_compute_slot_usages_many_uses_keep_twice);
    RUN_TEST(
        test_compute_slot_usages_equal_operands_count_two_uses_of_one_operation);
    RUN_TEST(
        test_compute_slot_usages_equal_operands_in_both_operations_are_twice);
    RUN_TEST(
        test_compute_slot_usages_equal_operands_and_a_multiply_use_are_twice);
    RUN_TEST(test_compute_slot_usages_two_boxes_two_adds_are_both_once);
    RUN_TEST(test_compute_slot_usages_two_boxes_two_multiplies_are_both_once);
    RUN_TEST(test_compute_slot_usages_two_boxes_one_add_one_multiply);
    RUN_TEST(test_compute_slot_usages_two_boxes_two_adds_one_multiply);
    RUN_TEST(test_compute_slot_usages_two_boxes_two_adds_two_multiplies);
    RUN_TEST(test_compute_slot_usages_two_boxes_three_adds_one_multiply);
    RUN_TEST(test_compute_slot_usages_two_boxes_one_multiply_only);
    RUN_TEST(test_compute_slot_usages_two_boxes_one_add_only);
    RUN_TEST(test_compute_slot_usages_two_boxes_without_uses_are_unmarked);
    RUN_TEST(test_compute_slot_usages_two_boxes_many_uses_are_both_twice);
    RUN_TEST(test_compute_slot_usages_three_boxes_follow_strip_order);
    RUN_TEST(
        test_compute_slot_usages_first_of_equal_slots_takes_the_single_use);
    RUN_TEST(test_compute_slot_usages_operation_none_contributes_nothing);
    RUN_TEST(test_compute_slot_usages_operation_none_does_not_hide_other_uses);
    RUN_TEST(
        test_compute_slot_usages_empty_smaller_box_counts_only_the_greater);
    RUN_TEST(
        test_compute_slot_usages_empty_greater_box_counts_only_the_smaller);
    RUN_TEST(
        test_compute_slot_usages_group_with_both_boxes_empty_marks_nothing);
    RUN_TEST(test_compute_slot_usages_invalid_group_still_counts);
    RUN_TEST(test_compute_slot_usages_empty_slot_is_never_marked);
    RUN_TEST(test_compute_slot_usages_empty_slots_do_not_match_each_other);
    RUN_TEST(test_compute_slot_usages_all_slots_empty_marks_nothing);
    RUN_TEST(test_compute_slot_usages_empty_slot_takes_no_rank);
    RUN_TEST(test_compute_slot_usages_empty_slot_between_equal_values);
    RUN_TEST(test_compute_slot_usages_range_boundaries_count);
    RUN_TEST(test_compute_slot_usages_out_of_range_operands_are_ignored);
    RUN_TEST(test_compute_slot_usages_out_of_range_partner_keeps_the_use);
    RUN_TEST(test_compute_slot_usages_out_of_range_strip_value_is_unmarked);
    RUN_TEST(test_compute_slot_usages_does_not_modify_its_inputs);
    RUN_TEST(test_compute_slot_usages_is_repeatable);
    RUN_TEST(test_compute_slot_usages_fixture_marks_every_slot_twice);
    RUN_TEST(test_compute_slot_usages_hidden_fixture_slots_are_unmarked);
    RUN_TEST(test_compute_slot_usages_fixture_duplicate_is_unmarked);
    return test_harness_result();
}
