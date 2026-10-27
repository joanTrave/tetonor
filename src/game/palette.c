#include "palette.h"

#define PALETTE_ENTRY(preferred, basic) {(preferred), (basic)}

/* The single place for theme data. */
static const Palette theme_palettes[PALETTE_THEME_COUNT] = {
    [PALETTE_THEME_CLASSIC] =
        {
            .foregrounds =
                {
                    [PALETTE_ROLE_ENTERED] = PALETTE_ENTRY(
                        PALETTE_COLOR_DARK_GREY, PALETTE_COLOR_YELLOW),
                    [PALETTE_ROLE_CORRECT] =
                        PALETTE_ENTRY(PALETTE_COLOR_GREEN, PALETTE_COLOR_GREEN),
                    [PALETTE_ROLE_WRONG] =
                        PALETTE_ENTRY(PALETTE_COLOR_RED, PALETTE_COLOR_RED),
                    [PALETTE_ROLE_USED_ONCE] =
                        PALETTE_ENTRY(PALETTE_COLOR_BLUE, PALETTE_COLOR_BLUE),
                    [PALETTE_ROLE_USED_TWICE] =
                        PALETTE_ENTRY(PALETTE_COLOR_GREEN, PALETTE_COLOR_GREEN),
                    [PALETTE_ROLE_USED_ONCE_TYPED] = PALETTE_ENTRY(
                        PALETTE_COLOR_BRIGHT_BLUE, PALETTE_COLOR_BLUE),
                    [PALETTE_ROLE_USED_TWICE_TYPED] = PALETTE_ENTRY(
                        PALETTE_COLOR_BRIGHT_GREEN, PALETTE_COLOR_GREEN),
                    [PALETTE_ROLE_TEXT] =
                        PALETTE_ENTRY(PALETTE_COLOR_WHITE, PALETTE_COLOR_WHITE),
                    [PALETTE_ROLE_CONFLICT] = PALETTE_ENTRY(
                        PALETTE_COLOR_MAGENTA, PALETTE_COLOR_MAGENTA),
                },
            .background =
                PALETTE_ENTRY(PALETTE_COLOR_BLACK, PALETTE_COLOR_BLACK),
        },
    [PALETTE_THEME_LIGHT] =
        {
            .foregrounds =
                {
                    [PALETTE_ROLE_ENTERED] = PALETTE_ENTRY(
                        PALETTE_COLOR_MAGENTA, PALETTE_COLOR_MAGENTA),
                    [PALETTE_ROLE_CORRECT] =
                        PALETTE_ENTRY(PALETTE_COLOR_GREEN, PALETTE_COLOR_GREEN),
                    [PALETTE_ROLE_WRONG] =
                        PALETTE_ENTRY(PALETTE_COLOR_RED, PALETTE_COLOR_RED),
                    [PALETTE_ROLE_USED_ONCE] =
                        PALETTE_ENTRY(PALETTE_COLOR_BLUE, PALETTE_COLOR_BLUE),
                    [PALETTE_ROLE_USED_TWICE] =
                        PALETTE_ENTRY(PALETTE_COLOR_GREEN, PALETTE_COLOR_GREEN),
                    [PALETTE_ROLE_USED_ONCE_TYPED] = PALETTE_ENTRY(
                        PALETTE_COLOR_BRIGHT_BLUE, PALETTE_COLOR_BLUE),
                    [PALETTE_ROLE_USED_TWICE_TYPED] =
                        PALETTE_ENTRY(PALETTE_COLOR_CYAN, PALETTE_COLOR_CYAN),
                    [PALETTE_ROLE_TEXT] =
                        PALETTE_ENTRY(PALETTE_COLOR_BLACK, PALETTE_COLOR_BLACK),
                    /* Every readable basic colour on the white background is
                     * taken, and yellow on white is unreadable, so on 8-colour
                     * terminals this shares red with WRONG (same error
                     * meaning; the "!" marker tells them apart). */
                    [PALETTE_ROLE_CONFLICT] = PALETTE_ENTRY(
                        PALETTE_COLOR_DARK_GREY, PALETTE_COLOR_RED),
                },
            .background =
                PALETTE_ENTRY(PALETTE_COLOR_BRIGHT_WHITE, PALETTE_COLOR_WHITE),
        },
};

bool is_palette_theme(int theme)
{
    return theme >= PALETTE_THEME_CLASSIC && theme < PALETTE_THEME_COUNT;
}

void init_theme_palette(Palette* palette, int theme)
{
    *palette =
        theme_palettes[is_palette_theme(theme) ? theme
                                               : TETONOR_DEFAULT_PALETTE_THEME];
}

void init_palette(Palette* palette)
{
    init_theme_palette(palette, TETONOR_DEFAULT_PALETTE_THEME);
}
