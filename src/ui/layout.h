#ifndef TETONOR_UI_LAYOUT_H
#define TETONOR_UI_LAYOUT_H

#include <stdbool.h>

#include "../game/operand_group.h"

/* Each matrix cell is a block: the matrix box on top and its three operand
 * boxes underneath, sharing one rule. Vertically adjacent blocks share their
 * border rule, so a block row takes LAYOUT_CELL_PITCH_Y lines.
 *
 *   +-------+  row 0   top rule (join rule in block rows 1-3)
 *   |  182  |  row 1   LAYOUT_CELL_VALUE_ROW
 *   +--+-+--+  row 2   LAYOUT_CELL_SPLIT_ROW
 *   |29|x| 7|  row 3   LAYOUT_CELL_GROUP_ROW
 *   +--+-+--+  row 4   next block's join rule (bottom rule in the last row)
 */
#define LAYOUT_ORIGIN_Y 1
#define LAYOUT_ORIGIN_X 2
#define LAYOUT_NUMBER_FIELD_W 2    /* inner width of a group number box */
#define LAYOUT_OPERATION_FIELD_W 1 /* inner width of the operation box */
#define LAYOUT_GROUP_RULES 4       /* vertical lines across a group row */
#define LAYOUT_CELL_W                                                          \
    (LAYOUT_GROUP_RULES + 2 * LAYOUT_NUMBER_FIELD_W + LAYOUT_OPERATION_FIELD_W)
#define LAYOUT_CELL_GAP_X 2   /* blank columns between blocks */
#define LAYOUT_CELL_PITCH_Y 4 /* lines per block row; borders are shared */
#define LAYOUT_CELL_VALUE_ROW 1
#define LAYOUT_CELL_SPLIT_ROW 2
#define LAYOUT_CELL_GROUP_ROW 3
#define LAYOUT_MINI_W 4
#define LAYOUT_MINI_H 3
#define LAYOUT_SECTION_GAP 1 /* the "Available:" label row */
#define LAYOUT_MESSAGE_GAP 1 /* blank rows above the result message */
#define LAYOUT_CORRECT_MESSAGE "CORRECT ANSWER"
#define LAYOUT_WRONG_MESSAGE "WRONG ANSWER"
#define LAYOUT_RESULT_MESSAGE_W ((int)(sizeof LAYOUT_CORRECT_MESSAGE - 1))
#define LAYOUT_UNDO_EMPTY_MESSAGE "Nothing to undo"
#define LAYOUT_NOTICE_GAP_X                                                    \
    2 /* blank columns between the result and a notice */
#define LAYOUT_CONFIRM_LABEL "CONFIRM"
#define LAYOUT_RESTART_LABEL "RESTART"
#define LAYOUT_BUTTON_PADDING 1 /* blank columns each side of the label */
#define LAYOUT_BUTTON_H 3
#define LAYOUT_BUTTON_W                                                        \
    ((int)(sizeof LAYOUT_CONFIRM_LABEL - 1) + 2 * (LAYOUT_BUTTON_PADDING + 1))
#define LAYOUT_BUTTON_GAP_X                                                    \
    2 /* columns between last available box and button */
#define LAYOUT_MIN_ROWS 27
#define LAYOUT_MIN_COLS 80
/* printf format: columns, then rows. */
#define LAYOUT_ENLARGE_FORMAT "Please enlarge the terminal to at least %dx%d"
#define LAYOUT_MENU_H 7
#define LAYOUT_MENU_W 50
#define LAYOUT_MENU_TITLE_ROW 1
#define LAYOUT_MENU_START_ROW 3
#define LAYOUT_MENU_EXIT_ROW 4
#define LAYOUT_MENU_TEXT_COL 2
#define LAYOUT_DIFFICULTY_MENU_H 10
#define LAYOUT_DIFFICULTY_MENU_W LAYOUT_MENU_W
#define LAYOUT_DIFFICULTY_TITLE_ROW 1
#define LAYOUT_DIFFICULTY_FIRST_OPTION_ROW 3
#define LAYOUT_DIFFICULTY_CHOOSE_ROW 7
#define LAYOUT_DIFFICULTY_BACK_ROW 8

typedef struct
{
    int y;
    int x;
} LayoutPoint;

/* (container - content) / 2, clamped to >= 0. */
int layout_centered_offset(int container_size, int content_size);

LayoutPoint layout_centered_window_origin(int screen_rows, int screen_cols,
                                          int height, int width);

/* Window row of a level's option line on the difficulty screen. */
int layout_difficulty_option_row(int difficulty);

/* Top-left corner of the block of the matrix cell at (row, col). */
LayoutPoint layout_sum_cell_origin(int row, int col);

/* Row of the bottom rule of the last block row. */
int layout_grid_bottom_row(void);

/* First inner column, on the group row, of one box of the cell's group. */
LayoutPoint layout_group_field_origin(int row, int col, GroupField field);

/* Inner width of one box of a group. */
int layout_group_field_width(GroupField field);

/* Row and column of the "Available:" label, above the available boxes. */
LayoutPoint layout_available_label_origin(void);
LayoutPoint layout_available_cell_origin(int index);

/* Top-left corner of the CONFIRM button, beside the available row. */
LayoutPoint layout_confirm_button_origin(void);

/* Top-left corner of the RESTART button, directly below CONFIRM. */
LayoutPoint layout_restart_button_origin(void);

/* Row/column of the result message, below the RESTART button. */
LayoutPoint layout_result_message_origin(void);

/* Row/column of the nothing-to-undo notice: on the result row, right of the
 * longer result text. */
LayoutPoint layout_undo_notice_origin(void);

/* True when a terminal of rows x cols holds the whole board. */
bool layout_fits(int rows, int cols);

#endif
