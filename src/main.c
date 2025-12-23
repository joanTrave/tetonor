#include <ncurses.h>

#include "ui/ui.h"

int main(void)
{
    GamePort* game_port = create_game_port();
    ui_run(game_port);
    destroy_game_port(game_port);
    return 0;
}
