#ifndef TETONOR_UI_LAYOUT_H
#define TETONOR_UI_LAYOUT_H

#define LAYOUT_ORIGIN_Y 2
#define LAYOUT_ORIGIN_X 4
#define LAYOUT_CELL_W 6
#define LAYOUT_CELL_H 3
#define LAYOUT_MINI_W 5
#define LAYOUT_MINI_H 3
#define LAYOUT_SECTION_GAP 2
#define LAYOUT_MESSAGE_GAP 1 /* blank rows above the result message */
#define LAYOUT_CONFIRM_LABEL "CONFIRM"
#define LAYOUT_BUTTON_PADDING 1 /* blank columns each side of the label */
#define LAYOUT_BUTTON_H 3
#define LAYOUT_BUTTON_W                                                        \
    ((int)(sizeof LAYOUT_CONFIRM_LABEL - 1) + 2 * (LAYOUT_BUTTON_PADDING + 1))
#define LAYOUT_BUTTON_GAP 1 /* blank rows between available row and button */
#define LAYOUT_MENU_H 7
#define LAYOUT_MENU_W 50
#define LAYOUT_MENU_TITLE_ROW 1
#define LAYOUT_MENU_START_ROW 3
#define LAYOUT_MENU_EXIT_ROW 4
#define LAYOUT_MENU_TEXT_COL 2

typedef struct
{
    int y;
    int x;
} LayoutPoint;

/* (container - content) / 2, clamped to >= 0. */
int layout_centered_offset(int container_size, int content_size);

LayoutPoint layout_centered_window_origin(int screen_rows, int screen_cols,
                                          int height, int width);
LayoutPoint layout_sum_cell_origin(int row, int col);
LayoutPoint layout_available_label_origin(void);
LayoutPoint layout_available_cell_origin(int index);

/* Top-left corner of the CONFIRM button, below the available-number boxes. */
LayoutPoint layout_confirm_button_origin(void);

/* Row/column of the result message, below the CONFIRM button. */
LayoutPoint layout_result_message_origin(void);

#endif
