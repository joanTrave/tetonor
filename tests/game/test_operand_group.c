#include <stdbool.h>

#include "../test_harness.h"
#include "../../src/game/number_generation.h"
#include "../../src/game/operand_group.h"

#define TEST_OUT_OF_RANGE_ABOVE (TETONOR_MAX_VALUE + 1)
#define TEST_OUT_OF_RANGE_BELOW (-5)

static OperandGroup make_group(int smaller, GroupOperation operation,
                               int greater)
{
    OperandGroup group = {smaller, operation, greater};
    return group;
}

static void test_clear_operand_group_empties_all_three_boxes(void)
{
    OperandGroup group = make_group(7, GROUP_OPERATION_MULTIPLY, 9);
    clear_operand_group(&group);
    ASSERT_EQ_INT(group.smaller, TETONOR_EMPTY_ENTRY);
    ASSERT_EQ_INT(group.operation, GROUP_OPERATION_NONE);
    ASSERT_EQ_INT(group.greater, TETONOR_EMPTY_ENTRY);
}

static void test_clear_operand_group_makes_group_not_filled(void)
{
    OperandGroup group = make_group(7, GROUP_OPERATION_ADD, 9);
    ASSERT_TRUE(is_operand_group_filled(&group));
    clear_operand_group(&group);
    ASSERT_FALSE(is_operand_group_filled(&group));
}

static void test_is_operand_group_filled_needs_all_three_boxes(void)
{
    OperandGroup complete = make_group(1, GROUP_OPERATION_ADD, 2);
    ASSERT_TRUE(is_operand_group_filled(&complete));

    OperandGroup no_smaller = complete;
    no_smaller.smaller      = TETONOR_EMPTY_ENTRY;
    ASSERT_FALSE(is_operand_group_filled(&no_smaller));

    OperandGroup no_operation = complete;
    no_operation.operation    = GROUP_OPERATION_NONE;
    ASSERT_FALSE(is_operand_group_filled(&no_operation));

    OperandGroup no_greater = complete;
    no_greater.greater      = TETONOR_EMPTY_ENTRY;
    ASSERT_FALSE(is_operand_group_filled(&no_greater));
}

static void test_is_operand_group_filled_accepts_zero_numbers(void)
{
    OperandGroup group = make_group(0, GROUP_OPERATION_MULTIPLY, 0);
    ASSERT_TRUE(is_operand_group_filled(&group));
}

static void test_is_operand_group_valid_accepts_addition(void)
{
    OperandGroup group = make_group(1, GROUP_OPERATION_ADD, 2);
    ASSERT_TRUE(is_operand_group_valid(&group, 3));
}

static void test_is_operand_group_valid_accepts_multiplication(void)
{
    OperandGroup group = make_group(3, GROUP_OPERATION_MULTIPLY, 4);
    ASSERT_TRUE(is_operand_group_valid(&group, 12));
}

static void test_is_operand_group_valid_distinguishes_the_operations(void)
{
    OperandGroup added      = make_group(3, GROUP_OPERATION_ADD, 4);
    OperandGroup multiplied = make_group(3, GROUP_OPERATION_MULTIPLY, 4);
    ASSERT_TRUE(is_operand_group_valid(&added, 7));
    ASSERT_FALSE(is_operand_group_valid(&added, 12));
    ASSERT_TRUE(is_operand_group_valid(&multiplied, 12));
    ASSERT_FALSE(is_operand_group_valid(&multiplied, 7));
}

static void test_is_operand_group_valid_allows_equal_numbers(void)
{
    OperandGroup added      = make_group(3, GROUP_OPERATION_ADD, 3);
    OperandGroup multiplied = make_group(2, GROUP_OPERATION_MULTIPLY, 2);
    ASSERT_TRUE(is_operand_group_valid(&added, 6));
    ASSERT_TRUE(is_operand_group_valid(&multiplied, 4));
}

static void test_is_operand_group_valid_rejects_smaller_above_greater(void)
{
    OperandGroup added      = make_group(2, GROUP_OPERATION_ADD, 1);
    OperandGroup multiplied = make_group(4, GROUP_OPERATION_MULTIPLY, 3);
    ASSERT_FALSE(is_operand_group_valid(&added, 3));
    ASSERT_FALSE(is_operand_group_valid(&multiplied, 12));
}

static void test_is_operand_group_valid_rejects_wrong_target(void)
{
    OperandGroup added      = make_group(1, GROUP_OPERATION_ADD, 2);
    OperandGroup multiplied = make_group(1, GROUP_OPERATION_MULTIPLY, 2);
    ASSERT_FALSE(is_operand_group_valid(&added, 4));
    ASSERT_FALSE(is_operand_group_valid(&added, 2));
    ASSERT_FALSE(is_operand_group_valid(&multiplied, 3));
    ASSERT_FALSE(is_operand_group_valid(&multiplied, 1));
}

static void test_is_operand_group_valid_rejects_missing_operation(void)
{
    OperandGroup group = make_group(0, GROUP_OPERATION_NONE, 0);
    ASSERT_FALSE(is_operand_group_valid(&group, 0));
    group = make_group(1, GROUP_OPERATION_NONE, 2);
    ASSERT_FALSE(is_operand_group_valid(&group, 3));
    ASSERT_FALSE(is_operand_group_valid(&group, 2));
}

static void test_is_operand_group_valid_rejects_empty_numbers(void)
{
    /* -1 + 5 == 4: the target would match if emptiness were read as a number.
     */
    OperandGroup no_smaller =
        make_group(TETONOR_EMPTY_ENTRY, GROUP_OPERATION_ADD, 5);
    OperandGroup no_greater =
        make_group(5, GROUP_OPERATION_ADD, TETONOR_EMPTY_ENTRY);
    OperandGroup none_at_all = make_group(
        TETONOR_EMPTY_ENTRY, GROUP_OPERATION_MULTIPLY, TETONOR_EMPTY_ENTRY);
    ASSERT_FALSE(is_operand_group_valid(&no_smaller, 4));
    ASSERT_FALSE(is_operand_group_valid(&no_greater, 4));
    ASSERT_FALSE(is_operand_group_valid(&none_at_all, 1));
}

static void test_is_operand_group_valid_rejects_cleared_group(void)
{
    OperandGroup group = make_group(1, GROUP_OPERATION_ADD, 2);
    clear_operand_group(&group);
    ASSERT_FALSE(is_operand_group_valid(&group, 0));
    ASSERT_FALSE(is_operand_group_valid(&group, TETONOR_EMPTY_ENTRY));
}

static void test_is_operand_group_valid_rejects_out_of_range_numbers(void)
{
    OperandGroup too_big =
        make_group(1, GROUP_OPERATION_ADD, TEST_OUT_OF_RANGE_ABOVE);
    OperandGroup too_small =
        make_group(TEST_OUT_OF_RANGE_BELOW, GROUP_OPERATION_ADD, 7);
    OperandGroup both_big =
        make_group(TEST_OUT_OF_RANGE_ABOVE, GROUP_OPERATION_MULTIPLY,
                   TEST_OUT_OF_RANGE_ABOVE);
    ASSERT_FALSE(is_operand_group_valid(&too_big, 1 + TEST_OUT_OF_RANGE_ABOVE));
    ASSERT_FALSE(is_operand_group_valid(&too_small, 2));
    ASSERT_FALSE(is_operand_group_valid(
        &both_big, TEST_OUT_OF_RANGE_ABOVE * TEST_OUT_OF_RANGE_ABOVE));
}

static void test_is_operand_group_valid_accepts_range_boundaries(void)
{
    OperandGroup zero_product = make_group(
        TETONOR_MIN_VALUE, GROUP_OPERATION_MULTIPLY, TETONOR_MAX_VALUE);
    OperandGroup zero_sum =
        make_group(TETONOR_MIN_VALUE, GROUP_OPERATION_ADD, TETONOR_MAX_VALUE);
    OperandGroup largest_sum =
        make_group(TETONOR_MAX_VALUE, GROUP_OPERATION_ADD, TETONOR_MAX_VALUE);
    OperandGroup largest_product = make_group(
        TETONOR_MAX_VALUE, GROUP_OPERATION_MULTIPLY, TETONOR_MAX_VALUE);
    ASSERT_TRUE(is_operand_group_valid(&zero_product, 0));
    ASSERT_TRUE(is_operand_group_valid(&zero_sum, TETONOR_MAX_VALUE));
    ASSERT_TRUE(is_operand_group_valid(&largest_sum, 2 * TETONOR_MAX_VALUE));
    ASSERT_TRUE(is_operand_group_valid(&largest_product,
                                       TETONOR_MAX_VALUE * TETONOR_MAX_VALUE));
}

static void test_is_operand_group_valid_does_not_modify_the_group(void)
{
    OperandGroup group = make_group(5, GROUP_OPERATION_ADD, 6);
    ASSERT_TRUE(is_operand_group_valid(&group, 11));
    ASSERT_EQ_INT(group.smaller, 5);
    ASSERT_EQ_INT(group.operation, GROUP_OPERATION_ADD);
    ASSERT_EQ_INT(group.greater, 6);
}

int main(void)
{
    RUN_TEST(test_clear_operand_group_empties_all_three_boxes);
    RUN_TEST(test_clear_operand_group_makes_group_not_filled);
    RUN_TEST(test_is_operand_group_filled_needs_all_three_boxes);
    RUN_TEST(test_is_operand_group_filled_accepts_zero_numbers);
    RUN_TEST(test_is_operand_group_valid_accepts_addition);
    RUN_TEST(test_is_operand_group_valid_accepts_multiplication);
    RUN_TEST(test_is_operand_group_valid_distinguishes_the_operations);
    RUN_TEST(test_is_operand_group_valid_allows_equal_numbers);
    RUN_TEST(test_is_operand_group_valid_rejects_smaller_above_greater);
    RUN_TEST(test_is_operand_group_valid_rejects_wrong_target);
    RUN_TEST(test_is_operand_group_valid_rejects_missing_operation);
    RUN_TEST(test_is_operand_group_valid_rejects_empty_numbers);
    RUN_TEST(test_is_operand_group_valid_rejects_cleared_group);
    RUN_TEST(test_is_operand_group_valid_rejects_out_of_range_numbers);
    RUN_TEST(test_is_operand_group_valid_accepts_range_boundaries);
    RUN_TEST(test_is_operand_group_valid_does_not_modify_the_group);
    return test_harness_result();
}
