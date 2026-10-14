#include <stdbool.h>

#include "style.h"
#include "palette_style.h"

#define TETONOR_FIRST_COLOR_PAIR 1
/* xterm-16 palette indexes of the extra colours. */
#define TETONOR_BRIGHT_OFFSET 8
#define TETONOR_COLOR_DARK_GREY 8
#define TETONOR_COLOR_BRIGHT_BLUE (COLOR_BLUE + TETONOR_BRIGHT_OFFSET)
#define TETONOR_COLOR_BRIGHT_GREEN (COLOR_GREEN + TETONOR_BRIGHT_OFFSET)

static short curses_color(PaletteColor color)
{
    switch (color)
    {
        case PALETTE_COLOR_BLACK:
            return COLOR_BLACK;
        case PALETTE_COLOR_RED:
            return COLOR_RED;
        case PALETTE_COLOR_GREEN:
            return COLOR_GREEN;
        case PALETTE_COLOR_YELLOW:
            return COLOR_YELLOW;
        case PALETTE_COLOR_BLUE:
            return COLOR_BLUE;
        case PALETTE_COLOR_MAGENTA:
            return COLOR_MAGENTA;
        case PALETTE_COLOR_CYAN:
            return COLOR_CYAN;
        case PALETTE_COLOR_WHITE:
            return COLOR_WHITE;
        case PALETTE_COLOR_DARK_GREY:
            return TETONOR_COLOR_DARK_GREY;
        case PALETTE_COLOR_BRIGHT_BLUE:
            return TETONOR_COLOR_BRIGHT_BLUE;
        case PALETTE_COLOR_BRIGHT_GREEN:
            return TETONOR_COLOR_BRIGHT_GREEN;
    }
    return COLOR_WHITE;
}

static bool colour_pairs_available(void)
{
    return has_colors() && COLOR_PAIRS > PALETTE_ROLE_COUNT;
}

static short curses_color_of(PaletteEntry entry)
{
    return curses_color(resolve_palette_color(entry, COLORS));
}

void init_styles(void)
{
    if (has_colors())
    {
        start_color();
    }
}

void apply_palette_styles(const Palette* palette)
{
    if (!colour_pairs_available())
    {
        return;
    }
    short background = curses_color_of(palette->background);
    for (int role = 0; role < PALETTE_ROLE_COUNT; role++)
    {
        init_pair((short)(role + TETONOR_FIRST_COLOR_PAIR),
                  curses_color_of(palette->foregrounds[role]), background);
    }
}

static attr_t monochrome_attr(PaletteRole role)
{
    switch (role)
    {
        case PALETTE_ROLE_ENTERED:
            return A_DIM | A_UNDERLINE;
        case PALETTE_ROLE_CORRECT:
        case PALETTE_ROLE_WRONG:
        case PALETTE_ROLE_USED_ONCE:
            return A_BOLD;
        case PALETTE_ROLE_USED_TWICE:
            return A_BOLD | A_UNDERLINE;
        case PALETTE_ROLE_USED_ONCE_TYPED:
            return A_BOLD | A_UNDERLINE | A_DIM;
        case PALETTE_ROLE_USED_TWICE_TYPED:
            return A_UNDERLINE;
        case PALETTE_ROLE_COUNT:
            break;
    }
    return A_NORMAL;
}

attr_t palette_role_attr(PaletteRole role)
{
    if (!colour_pairs_available())
    {
        return monochrome_attr(role);
    }
    /* No A_BOLD here: some Windows backends turn bold into an intensity
     * change that alters the hue, so the pair alone carries the colour. */
    return COLOR_PAIR(role + TETONOR_FIRST_COLOR_PAIR);
}
