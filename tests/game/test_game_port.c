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

int main(void)
{
    RUN_TEST(test_create_game_port_returns_non_null);
    RUN_TEST(test_create_game_port_sets_default_difficulty);
    RUN_TEST(test_create_game_port_wires_number_generation_functions);
    RUN_TEST(test_game_port_generates_and_frees_through_port);
    RUN_TEST(test_destroy_game_port_accepts_null);
    return test_harness_result();
}
