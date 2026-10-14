#include <stdbool.h>
#include <stdlib.h>

#include "../test_harness.h"
#include "../../src/game/game_port.h"

#define TEST_SEED 42

static int count_hidden(const NumberGeneration* number_generation)
{
    int hidden = 0;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (number_generation->available_array[i] == TETONOR_HIDDEN_NUMBER)
        {
            hidden++;
        }
    }
    return hidden;
}

static void test_create_game_port_returns_non_null(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    destroy_game_port(game_port);
}

static void test_create_game_port_sets_default_difficulty(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    int difficulty = game_port->difficulty;
    destroy_game_port(game_port);
    ASSERT_EQ_INT(difficulty, TETONOR_DEFAULT_DIFFICULTY);
}

static void test_create_game_port_wires_number_generation_functions(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    int generate_wired =
        game_port->generate_random_numbers_f == generate_random_numbers;
    int free_wired =
        game_port->free_number_generation_f == free_number_generation;
    destroy_game_port(game_port);
    ASSERT_TRUE(generate_wired);
    ASSERT_TRUE(free_wired);
}

static void test_game_port_generates_and_frees_through_port(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    srand(TEST_SEED);
    NumberGeneration* number_generation =
        game_port->generate_random_numbers_f(game_port->difficulty);
    if (number_generation == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(number_generation);
    }
    int hidden     = count_hidden(number_generation);
    int max_hidden = game_port->difficulty + TETONOR_HIDDEN_BASE_COUNT;
    game_port->free_number_generation_f(number_generation);
    destroy_game_port(game_port);
    ASSERT_TRUE(hidden >= 1);
    ASSERT_TRUE(hidden <= max_hidden);
}

static void test_destroy_game_port_accepts_null(void)
{
    destroy_game_port(NULL);
    ASSERT_TRUE(1);
}

static void press_digit_through_port(const GamePort* game_port, Board* board,
                                     int digit)
{
    BoardInput input = {BOARD_INPUT_DIGIT, digit};
    game_port->apply_board_input_f(board, input);
}

static void press_through_port(const GamePort* game_port, Board* board,
                               BoardInputKind kind)
{
    BoardInput input = {kind, 0};
    game_port->apply_board_input_f(board, input);
}

/* Types every hidden slot's solution value, adding first_slot_offset to the
 * first one (modulo the value range) to make the answer wrong on demand. The
 * RIGHT after the last slot focuses the submit control, which ACTIVATE then
 * presses. */
static void play_session(const GamePort* game_port, Board* board,
                         const NumberGeneration* puzzle, int first_slot_offset)
{
    bool first = true;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (puzzle->available_array[i] != TETONOR_HIDDEN_NUMBER)
        {
            continue;
        }
        int value = puzzle->solution_array[i];
        if (first)
        {
            value = (value + first_slot_offset) % (TETONOR_MAX_VALUE + 1);
            first = false;
        }
        if (value >= TETONOR_DIGIT_BASE)
        {
            press_digit_through_port(game_port, board,
                                     value / TETONOR_DIGIT_BASE);
        }
        press_digit_through_port(game_port, board, value % TETONOR_DIGIT_BASE);
        press_through_port(game_port, board, BOARD_INPUT_MOVE_RIGHT);
    }
    press_through_port(game_port, board, BOARD_INPUT_ACTIVATE);
}

static void test_create_game_port_wires_board_functions(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    int init_wired  = game_port->init_board_f == init_board;
    int apply_wired = game_port->apply_board_input_f == apply_board_input;
    destroy_game_port(game_port);
    ASSERT_TRUE(init_wired);
    ASSERT_TRUE(apply_wired);
}

static void test_game_port_board_session_through_port_is_correct(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    srand(TEST_SEED);
    NumberGeneration* number_generation =
        game_port->generate_random_numbers_f(game_port->difficulty);
    if (number_generation == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(number_generation);
    }
    Board board;
    game_port->init_board_f(&board, number_generation);
    BoardResult before = board.result;
    play_session(game_port, &board, number_generation, 0);
    BoardResult after = board.result;
    game_port->free_number_generation_f(number_generation);
    destroy_game_port(game_port);
    ASSERT_EQ_INT(before, BOARD_RESULT_PENDING);
    ASSERT_EQ_INT(after, BOARD_RESULT_CORRECT);
}

static void test_game_port_board_session_through_port_is_wrong(void)
{
    GamePort* game_port = create_game_port();
    ASSERT_NOT_NULL(game_port);
    srand(TEST_SEED);
    NumberGeneration* number_generation =
        game_port->generate_random_numbers_f(game_port->difficulty);
    if (number_generation == NULL)
    {
        destroy_game_port(game_port);
        ASSERT_NOT_NULL(number_generation);
    }
    Board board;
    game_port->init_board_f(&board, number_generation);
    play_session(game_port, &board, number_generation, 1);
    BoardResult result = board.result;
    game_port->free_number_generation_f(number_generation);
    destroy_game_port(game_port);
    ASSERT_EQ_INT(result, BOARD_RESULT_WRONG);
}

int main(void)
{
    RUN_TEST(test_create_game_port_returns_non_null);
    RUN_TEST(test_create_game_port_sets_default_difficulty);
    RUN_TEST(test_create_game_port_wires_number_generation_functions);
    RUN_TEST(test_game_port_generates_and_frees_through_port);
    RUN_TEST(test_destroy_game_port_accepts_null);
    RUN_TEST(test_create_game_port_wires_board_functions);
    RUN_TEST(test_game_port_board_session_through_port_is_correct);
    RUN_TEST(test_game_port_board_session_through_port_is_wrong);
    return test_harness_result();
}
