#include <stdlib.h>

#include "game_port.h"

GamePort* create_game_port(void)
{
    GamePort* game_port = malloc(sizeof *game_port);
    if (game_port == NULL)
    {
        return NULL;
    }
    game_port->difficulty                     = TETONOR_DEFAULT_DIFFICULTY;
    game_port->generate_random_numbers_f      = &generate_random_numbers;
    game_port->free_number_generation_f       = &free_number_generation;
    game_port->init_board_f                   = &init_board;
    game_port->apply_board_input_f            = &apply_board_input;
    game_port->undo_board_move_f              = &undo_board_move;
    game_port->is_board_locked_f              = &is_board_locked;
    game_port->board_progress_would_be_lost_f = &board_progress_would_be_lost;
    game_port->board_move_count_f             = &board_move_count;
    game_port->request_board_hint_f           = &request_board_hint;
    game_port->board_counts_for_statistics_f  = &board_counts_for_statistics;
    game_port->start_game_timer_f             = &start_game_timer;
    game_port->update_game_timer_f            = &update_game_timer;
    game_port->pause_game_timer_f             = &pause_game_timer;
    game_port->resume_game_timer_f            = &resume_game_timer;
    game_port->capture_saved_game_f           = &capture_saved_game;
    game_port->restore_saved_game_f           = &restore_saved_game;
    return game_port;
}

void destroy_game_port(GamePort* game_port)
{
    free(game_port);
}

bool restart_game(const GamePort* game_port, Board* board,
                  NumberGeneration** puzzle)
{
    NumberGeneration* next =
        game_port->generate_random_numbers_f(game_port->difficulty);
    if (next == NULL)
    {
        return false;
    }
    game_port->init_board_f(board, next);
    game_port->free_number_generation_f(*puzzle);
    *puzzle = next;
    return true;
}
