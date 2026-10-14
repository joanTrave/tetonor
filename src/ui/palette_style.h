#ifndef TETONOR_UI_PALETTE_STYLE_H
#define TETONOR_UI_PALETTE_STYLE_H

#include <stdbool.h>

#include "../game/game_port.h"

#define TETONOR_BASIC_COLOR_COUNT 8 /* colours every colour terminal has */
#define TETONOR_16_COLOR_COUNT                                                 \
    16 /* needed by DARK_GREY and the bright colours */

/* entry's colour when a terminal with color_count colours can show it, else
 * its fallback. */
PaletteColor resolve_palette_color(PaletteEntry entry, int color_count);

/* Role used to draw a filled strip slot's number. Returns false when the
 * number keeps the default attribute (given, unused). */
bool strip_slot_role(bool editable, SlotUsage usage, PaletteRole* role);

#endif
