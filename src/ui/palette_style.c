#include "palette_style.h"

/* Smallest terminal colour count that can show color. */
static int required_color_count(PaletteColor color)
{
    switch (color)
    {
        case PALETTE_COLOR_BLACK:
        case PALETTE_COLOR_RED:
        case PALETTE_COLOR_GREEN:
        case PALETTE_COLOR_YELLOW:
        case PALETTE_COLOR_BLUE:
        case PALETTE_COLOR_MAGENTA:
        case PALETTE_COLOR_CYAN:
        case PALETTE_COLOR_WHITE:
            return TETONOR_BASIC_COLOR_COUNT;
        case PALETTE_COLOR_DARK_GREY:
        case PALETTE_COLOR_BRIGHT_BLUE:
        case PALETTE_COLOR_BRIGHT_GREEN:
            return TETONOR_16_COLOR_COUNT;
    }
    return TETONOR_16_COLOR_COUNT;
}

PaletteColor resolve_palette_color(PaletteEntry entry, int color_count)
{
    return color_count >= required_color_count(entry.color) ? entry.color
                                                            : entry.fallback;
}

bool strip_slot_role(bool editable, SlotUsage usage, PaletteRole* role)
{
    switch (usage)
    {
        case SLOT_USAGE_NONE:
            if (!editable)
            {
                return false;
            }
            *role = PALETTE_ROLE_ENTERED;
            return true;
        case SLOT_USAGE_ONCE:
            *role = editable ? PALETTE_ROLE_USED_ONCE_TYPED
                             : PALETTE_ROLE_USED_ONCE;
            return true;
        case SLOT_USAGE_TWICE:
            *role = editable ? PALETTE_ROLE_USED_TWICE_TYPED
                             : PALETTE_ROLE_USED_TWICE;
            return true;
    }
    return false;
}
