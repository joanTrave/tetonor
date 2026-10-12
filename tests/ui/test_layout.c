#include "../test_harness.h"
#include "../../src/ui/layout.h"

#define ASSERT_POINT(point, expected_y, expected_x)                            \
    do                                                                         \
    {                                                                          \
        LayoutPoint harness_point = (point);                                   \
        ASSERT_EQ_INT(harness_point.y, expected_y);                            \
        ASSERT_EQ_INT(harness_point.x, expected_x);                            \
    } while (0)

static void test_layout_centered_offset_centres_content(void)
{
    ASSERT_EQ_INT(layout_centered_offset(50, 18), 16);
    ASSERT_EQ_INT(layout_centered_offset(10, 4), 3);
    ASSERT_EQ_INT(layout_centered_offset(11, 4), 3);
}

static void test_layout_centered_offset_equal_sizes_is_zero(void)
{
    ASSERT_EQ_INT(layout_centered_offset(7, 7), 0);
}

static void test_layout_centered_offset_clamps_oversized_content_to_zero(void)
{
    ASSERT_EQ_INT(layout_centered_offset(5, 9), 0);
}

static void test_layout_centered_window_origin_centres_menu(void)
{
    ASSERT_POINT(layout_centered_window_origin(24, 80, 7, 50), 8, 15);
}

static void test_layout_centered_window_origin_clamps_small_terminal(void)
{
    ASSERT_POINT(layout_centered_window_origin(5, 40, 7, 50), 0, 0);
}

static void test_layout_sum_cell_origin_first_and_last_cell(void)
{
    ASSERT_POINT(layout_sum_cell_origin(0, 0), 2, 4);
    ASSERT_POINT(layout_sum_cell_origin(3, 3), 11, 22);
}

static void test_layout_available_label_origin_above_cells(void)
{
    ASSERT_POINT(layout_available_label_origin(), 15, 4);
}

static void test_layout_available_cell_origin_first_and_last(void)
{
    ASSERT_POINT(layout_available_cell_origin(0), 16, 4);
    ASSERT_POINT(layout_available_cell_origin(15), 16, 79);
}

static void test_layout_result_message_origin_below_available_row(void)
{
    LayoutPoint origin = layout_result_message_origin();
    ASSERT_EQ_INT(origin.y, layout_available_cell_origin(0).y + LAYOUT_MINI_H +
                                LAYOUT_MESSAGE_GAP);
    ASSERT_EQ_INT(origin.x, LAYOUT_ORIGIN_X);
    ASSERT_POINT(origin, 20, 4);
}

int main(void)
{
    RUN_TEST(test_layout_centered_offset_centres_content);
    RUN_TEST(test_layout_centered_offset_equal_sizes_is_zero);
    RUN_TEST(test_layout_centered_offset_clamps_oversized_content_to_zero);
    RUN_TEST(test_layout_centered_window_origin_centres_menu);
    RUN_TEST(test_layout_centered_window_origin_clamps_small_terminal);
    RUN_TEST(test_layout_sum_cell_origin_first_and_last_cell);
    RUN_TEST(test_layout_available_label_origin_above_cells);
    RUN_TEST(test_layout_available_cell_origin_first_and_last);
    RUN_TEST(test_layout_result_message_origin_below_available_row);
    return test_harness_result();
}
