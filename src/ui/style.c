#include <stdbool.h>

#include "style.h"

#define TETONOR_FIRST_COLOR_PAIR 1
#define TETONOR_COLOR_DARK_GREY 8

static short style_foreground(TextStyle style)
{
    switch (style)
    {
        case TEXT_STYLE_CORRECT:
            return COLOR_GREEN;
        case TEXT_STYLE_WRONG:
            return COLOR_RED;
        case TEXT_STYLE_ENTERED:
        case TEXT_STYLE_COUNT:
            break;
    }
    return COLORS > TETONOR_COLOR_DARK_GREY ? TETONOR_COLOR_DARK_GREY
                                            : COLOR_BLUE;
}

static bool colour_pairs_available(void)
{
    return has_colors() && COLOR_PAIRS > TEXT_STYLE_COUNT;
}

void init_styles(void)
{
    if (!has_colors() || start_color() == ERR)
    {
        return;
    }
    for (int style = 0; style < TEXT_STYLE_COUNT; style++)
    {
        init_pair((short)(style + TETONOR_FIRST_COLOR_PAIR),
                  style_foreground((TextStyle)style), COLOR_BLACK);
    }
}

attr_t text_style_attr(TextStyle style)
{
    if (colour_pairs_available())
    {
        /* No A_BOLD here: some Windows backends turn bold into an intensity
         * change that alters the hue, so the pair alone carries the colour. */
        return COLOR_PAIR(style + TETONOR_FIRST_COLOR_PAIR);
    }
    return style == TEXT_STYLE_ENTERED ? (A_DIM | A_UNDERLINE) : A_BOLD;
}
