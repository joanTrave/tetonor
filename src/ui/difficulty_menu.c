#include "difficulty_menu.h"

static const char* const difficulty_labels[DIFFICULTY_COUNT] = {
    [DIFFICULTY_EASY]   = "Easy",
    [DIFFICULTY_MEDIUM] = "Medium",
    [DIFFICULTY_HARD]   = "Hard",
};

static void apply_open_command(DifficultyMenu* menu,
                               DifficultyMenuCommand command)
{
    switch (command)
    {
        case DIFFICULTY_COMMAND_UP:
            if (menu->selected > DIFFICULTY_EASY)
            {
                menu->selected--;
            }
            break;
        case DIFFICULTY_COMMAND_DOWN:
            if (menu->selected < DIFFICULTY_HARD)
            {
                menu->selected++;
            }
            break;
        case DIFFICULTY_COMMAND_CONFIRM:
            menu->status = DIFFICULTY_MENU_CONFIRMED;
            break;
        case DIFFICULTY_COMMAND_BACK:
            menu->status = DIFFICULTY_MENU_CANCELLED;
            break;
        case DIFFICULTY_COMMAND_NONE:
            break;
    }
}

void init_difficulty_menu(DifficultyMenu* menu)
{
    menu->selected = DIFFICULTY_MEDIUM;
    menu->status   = DIFFICULTY_MENU_OPEN;
}

void apply_difficulty_menu_command(DifficultyMenu* menu,
                                   DifficultyMenuCommand command)
{
    switch (menu->status)
    {
        case DIFFICULTY_MENU_OPEN:
            apply_open_command(menu, command);
            break;
        case DIFFICULTY_MENU_CONFIRMED:
        case DIFFICULTY_MENU_CANCELLED:
            break;
    }
}

int difficulty_menu_clamp(int difficulty)
{
    if (difficulty < DIFFICULTY_EASY)
    {
        return DIFFICULTY_EASY;
    }
    if (difficulty > DIFFICULTY_HARD)
    {
        return DIFFICULTY_HARD;
    }
    return difficulty;
}

const char* difficulty_menu_label(int difficulty)
{
    return difficulty_labels[difficulty_menu_clamp(difficulty)];
}
