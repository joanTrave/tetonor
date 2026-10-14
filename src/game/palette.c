#include "palette.h"

static void set_entry(PaletteEntry* entry, PaletteColor color,
                      PaletteColor fallback)
{
    entry->color    = color;
    entry->fallback = fallback;
}

void init_palette(Palette* palette)
{
    PaletteEntry* roles = palette->foregrounds;
    set_entry(&roles[PALETTE_ROLE_ENTERED], PALETTE_COLOR_DARK_GREY,
              PALETTE_COLOR_YELLOW);
    set_entry(&roles[PALETTE_ROLE_CORRECT], PALETTE_COLOR_GREEN,
              PALETTE_COLOR_GREEN);
    set_entry(&roles[PALETTE_ROLE_WRONG], PALETTE_COLOR_RED, PALETTE_COLOR_RED);
    set_entry(&roles[PALETTE_ROLE_USED_ONCE], PALETTE_COLOR_BLUE,
              PALETTE_COLOR_BLUE);
    set_entry(&roles[PALETTE_ROLE_USED_TWICE], PALETTE_COLOR_GREEN,
              PALETTE_COLOR_GREEN);
    set_entry(&roles[PALETTE_ROLE_USED_ONCE_TYPED], PALETTE_COLOR_BRIGHT_BLUE,
              PALETTE_COLOR_BLUE);
    set_entry(&roles[PALETTE_ROLE_USED_TWICE_TYPED], PALETTE_COLOR_BRIGHT_GREEN,
              PALETTE_COLOR_GREEN);
    set_entry(&palette->background, PALETTE_COLOR_BLACK, PALETTE_COLOR_BLACK);
}
