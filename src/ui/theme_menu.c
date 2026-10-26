#include "theme_menu.h"

static const char* const theme_labels[PALETTE_THEME_COUNT] = {
    [PALETTE_THEME_CLASSIC] = "Classic",
    [PALETTE_THEME_LIGHT]   = "Light",
};

static int clamp_theme(int theme)
{
    if (theme < PALETTE_THEME_CLASSIC)
    {
        return PALETTE_THEME_CLASSIC;
    }
    if (theme > PALETTE_THEME_COUNT - 1)
    {
        return PALETTE_THEME_COUNT - 1;
    }
    return theme;
}

static void apply_open_command(ThemeMenu* menu, ThemeMenuCommand command)
{
    switch (command)
    {
        case THEME_MENU_COMMAND_UP:
            if (menu->selected > PALETTE_THEME_CLASSIC)
            {
                menu->selected--;
            }
            break;
        case THEME_MENU_COMMAND_DOWN:
            if (menu->selected < PALETTE_THEME_COUNT - 1)
            {
                menu->selected++;
            }
            break;
        case THEME_MENU_COMMAND_SELECT:
            menu->status = THEME_MENU_CHOSEN;
            break;
        case THEME_MENU_COMMAND_BACK:
            menu->status = THEME_MENU_CANCELLED;
            break;
        case THEME_MENU_COMMAND_NONE:
            break;
    }
}

void init_theme_menu(ThemeMenu* menu, int active_theme)
{
    menu->active   = is_palette_theme(active_theme)
                         ? (PaletteTheme)active_theme
                         : TETONOR_DEFAULT_PALETTE_THEME;
    menu->selected = menu->active;
    menu->status   = THEME_MENU_OPEN;
}

void apply_theme_menu_command(ThemeMenu* menu, ThemeMenuCommand command)
{
    switch (menu->status)
    {
        case THEME_MENU_OPEN:
            apply_open_command(menu, command);
            break;
        case THEME_MENU_CHOSEN:
        case THEME_MENU_CANCELLED:
            break;
    }
}

const char* theme_menu_label(int theme)
{
    return theme_labels[clamp_theme(theme)];
}

const char* theme_menu_marker(const ThemeMenu* menu, int theme)
{
    return theme == (int)menu->active ? THEME_MENU_ACTIVE_MARKER : "";
}
