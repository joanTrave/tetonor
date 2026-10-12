#include "layout.h"
#include "../game/game_port.h"

int layout_centered_offset(int container_size, int content_size)
{
    int offset = (container_size - content_size) / 2;
    return offset > 0 ? offset : 0;
}

LayoutPoint layout_centered_window_origin(int screen_rows, int screen_cols,
                                          int height, int width)
{
    LayoutPoint origin;
    origin.y = layout_centered_offset(screen_rows, height);
    origin.x = layout_centered_offset(screen_cols, width);
    return origin;
}

LayoutPoint layout_sum_cell_origin(int row, int col)
{
    LayoutPoint origin;
    origin.y = LAYOUT_ORIGIN_Y + row * LAYOUT_CELL_H;
    origin.x = LAYOUT_ORIGIN_X + col * LAYOUT_CELL_W;
    return origin;
}

LayoutPoint layout_available_label_origin(void)
{
    LayoutPoint origin = layout_available_cell_origin(0);
    origin.y -= 1;
    return origin;
}

LayoutPoint layout_available_cell_origin(int index)
{
    LayoutPoint origin;
    origin.y = LAYOUT_ORIGIN_Y + TETONOR_GRID_SIZE * LAYOUT_CELL_H +
               LAYOUT_SECTION_GAP;
    origin.x = LAYOUT_ORIGIN_X + index * LAYOUT_MINI_W;
    return origin;
}

LayoutPoint layout_result_message_origin(void)
{
    LayoutPoint origin;
    origin.y =
        layout_available_cell_origin(0).y + LAYOUT_MINI_H + LAYOUT_MESSAGE_GAP;
    origin.x = LAYOUT_ORIGIN_X;
    return origin;
}
