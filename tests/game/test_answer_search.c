#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../test_harness.h"
#include "fixture_puzzle.h"
#include "fixture_saved_game.h"
#include "../../src/game/answer_check.h"
#include "../../src/game/answer_search.h"
#include "../../src/game/conflict_check.h"

#define TEST_SEED_COUNT 30
#define TEST_LEVEL_COUNT 3
#define TEST_UNTOUCHED_BYTE 0x5a
#define TEST_OUT_OF_RANGE_HIGH (TETONOR_MAX_VALUE + 1)
#define TEST_OUT_OF_RANGE_LOW (-2)
#define TEST_BIG_GROUP_NUMBER 99
#define TEST_LCG_MULTIPLIER 1103515245u
#define TEST_LCG_INCREMENT 12345u
#define TEST_LCG_SHIFT 16
#define TEST_REVEAL_ODDS 3 /* one box in three is kept as a player entry */
#define TEST_WIDE_SUM_TARGET 5
#define TEST_WIDE_PRODUCT_TARGET 6

static const int test_levels[TEST_LEVEL_COUNT] = {
    DIFFICULTY_EASY, DIFFICULTY_MEDIUM, DIFFICULTY_HARD};

/* The wide puzzle's cells only admit the pairs (2,3) and (1,5), and (2,3) takes
 * one 2 and one 3, so a strip with five 2s and three 3s is no answer. Every
 * value stays within the capacity bound, so only the search can tell. */
static const int wide_strip[TETONOR_CELL_COUNT] = {1, 1, 1, 1, 2, 2, 2, 2,
                                                   2, 3, 3, 3, 5, 5, 5, 5};

typedef struct
{
    NumberGeneration puzzle;
    int strip[TETONOR_CELL_COUNT];
    OperandGroup groups[TETONOR_CELL_COUNT];
    int completed_strip[TETONOR_CELL_COUNT];
    OperandGroup completed_groups[TETONOR_CELL_COUNT];
} SearchCase;

static void strip_from_puzzle(const NumberGeneration* puzzle,
                              int strip[TETONOR_CELL_COUNT])
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        strip[i] = puzzle->available_array[i] == TETONOR_HIDDEN_NUMBER
                       ? TETONOR_EMPTY_ENTRY
                       : puzzle->available_array[i];
    }
}

static void clear_groups(OperandGroup groups[TETONOR_CELL_COUNT])
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        clear_operand_group(&groups[i]);
    }
}

/* F-S at the given level with nothing typed. */
static void start_save_case(SearchCase* search_case, int difficulty)
{
    build_save_fixture_puzzle(&search_case->puzzle, difficulty);
    strip_from_puzzle(&search_case->puzzle, search_case->strip);
    clear_groups(search_case->groups);
}

static void set_group(SearchCase* search_case, int cell, int smaller,
                      GroupOperation operation, int greater)
{
    search_case->groups[cell] = (OperandGroup){smaller, operation, greater};
}

static bool run_search(SearchCase* search_case)
{
    return find_answer_completion(
        &search_case->puzzle, search_case->strip, search_case->groups,
        search_case->completed_strip, search_case->completed_groups);
}

static bool completion_keeps_entries(const SearchCase* search_case)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (search_case->strip[i] != TETONOR_EMPTY_ENTRY &&
            search_case->completed_strip[i] != search_case->strip[i])
        {
            return false;
        }
        const OperandGroup* entry = &search_case->groups[i];
        const OperandGroup* done  = &search_case->completed_groups[i];
        if ((entry->smaller != TETONOR_EMPTY_ENTRY &&
             entry->smaller != done->smaller) ||
            (entry->operation != GROUP_OPERATION_NONE &&
             entry->operation != done->operation) ||
            (entry->greater != TETONOR_EMPTY_ENTRY &&
             entry->greater != done->greater))
        {
            return false;
        }
    }
    return true;
}

static bool completion_is_correct(const SearchCase* search_case)
{
    return is_answer_correct(&search_case->puzzle, search_case->completed_strip,
                             search_case->completed_groups);
}

static void poison_outputs(SearchCase* search_case)
{
    memset(search_case->completed_strip, TEST_UNTOUCHED_BYTE,
           sizeof search_case->completed_strip);
    memset(search_case->completed_groups, TEST_UNTOUCHED_BYTE,
           sizeof search_case->completed_groups);
}

static bool outputs_are_poisoned(const SearchCase* search_case)
{
    const unsigned char* strip_bytes =
        (const unsigned char*)search_case->completed_strip;
    const unsigned char* group_bytes =
        (const unsigned char*)search_case->completed_groups;
    for (size_t i = 0; i < sizeof search_case->completed_strip; i++)
    {
        if (strip_bytes[i] != TEST_UNTOUCHED_BYTE)
        {
            return false;
        }
    }
    for (size_t i = 0; i < sizeof search_case->completed_groups; i++)
    {
        if (group_bytes[i] != TEST_UNTOUCHED_BYTE)
        {
            return false;
        }
    }
    return true;
}

static void expect_search_fails_untouched(SearchCase* search_case)
{
    poison_outputs(search_case);
    ASSERT_FALSE(run_search(search_case));
    ASSERT_TRUE(outputs_are_poisoned(search_case));
}

static unsigned next_random(unsigned* state)
{
    *state = *state * TEST_LCG_MULTIPLIER + TEST_LCG_INCREMENT;
    return *state >> TEST_LCG_SHIFT;
}

static bool is_revealed(unsigned* state)
{
    return next_random(state) % TEST_REVEAL_ODDS == 0;
}

/* The entries a player who follows the generator's own solution could have
 * typed: a pseudo-random subset of its boxes. */
static void reveal_part_of_the_solution(SearchCase* search_case, unsigned seed)
{
    unsigned state                 = seed;
    const NumberGeneration* puzzle = &search_case->puzzle;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (puzzle->available_array[i] == TETONOR_HIDDEN_NUMBER &&
            is_revealed(&state))
        {
            search_case->strip[i] = puzzle->solution_array[i];
        }
        const OperandGroup* solution = &puzzle->solution_groups[i];
        OperandGroup* entry          = &search_case->groups[i];
        if (is_revealed(&state))
        {
            entry->smaller = solution->smaller;
        }
        if (is_revealed(&state))
        {
            entry->operation = solution->operation;
        }
        if (is_revealed(&state))
        {
            entry->greater = solution->greater;
        }
    }
}

static void test_find_answer_completion_empty_board_is_correct(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
        {
            srand(seed);
            NumberGeneration* generated =
                generate_random_numbers(test_levels[level]);
            ASSERT_NOT_NULL(generated);
            SearchCase search_case;
            search_case.puzzle = *generated;
            free_number_generation(generated);
            strip_from_puzzle(&search_case.puzzle, search_case.strip);
            clear_groups(search_case.groups);
            poison_outputs(&search_case);
            ASSERT_TRUE(run_search(&search_case));
            ASSERT_TRUE(completion_is_correct(&search_case));
            ASSERT_TRUE(completion_keeps_entries(&search_case));
        }
    }
}

static void test_find_answer_completion_keeps_given_and_typed_entries(void)
{
    SearchCase search_case;
    start_save_case(&search_case, DIFFICULTY_EASY);
    search_case.strip[1]  = 1;
    search_case.strip[5]  = 3;
    search_case.strip[8]  = 5;
    search_case.strip[12] = 9;
    set_group(&search_case, 0, 1, GROUP_OPERATION_NONE, TETONOR_EMPTY_ENTRY);
    set_group(&search_case, 1, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_MULTIPLY,
              TETONOR_EMPTY_ENTRY);
    set_group(&search_case, 4, 5, GROUP_OPERATION_ADD, 6);
    set_group(&search_case, 5, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_NONE, 6);
    set_group(&search_case, 7, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_MULTIPLY,
              TETONOR_EMPTY_ENTRY);
    set_group(&search_case, 14, 2, GROUP_OPERATION_NONE, TETONOR_EMPTY_ENTRY);
    poison_outputs(&search_case);
    ASSERT_TRUE(run_search(&search_case));
    ASSERT_TRUE(completion_keeps_entries(&search_case));
    ASSERT_TRUE(completion_is_correct(&search_case));
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (search_case.puzzle.available_array[i] != TETONOR_HIDDEN_NUMBER)
        {
            ASSERT_EQ_INT(search_case.completed_strip[i],
                          search_case.puzzle.available_array[i]);
        }
    }
}

static void test_find_answer_completion_keeps_part_of_a_known_solution(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
        {
            srand(seed);
            NumberGeneration* generated =
                generate_random_numbers(test_levels[level]);
            ASSERT_NOT_NULL(generated);
            SearchCase search_case;
            search_case.puzzle = *generated;
            free_number_generation(generated);
            strip_from_puzzle(&search_case.puzzle, search_case.strip);
            clear_groups(search_case.groups);
            reveal_part_of_the_solution(&search_case, seed);
            poison_outputs(&search_case);
            ASSERT_TRUE(run_search(&search_case));
            ASSERT_TRUE(completion_keeps_entries(&search_case));
            ASSERT_TRUE(completion_is_correct(&search_case));
        }
    }
}

static void test_find_answer_completion_returns_a_full_solution_unchanged(void)
{
    SearchCase search_case;
    start_save_case(&search_case, DIFFICULTY_MEDIUM);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        search_case.strip[i]  = search_case.puzzle.solution_array[i];
        search_case.groups[i] = search_case.puzzle.solution_groups[i];
    }
    poison_outputs(&search_case);
    ASSERT_TRUE(run_search(&search_case));
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        ASSERT_EQ_INT(search_case.completed_strip[i], search_case.strip[i]);
        ASSERT_EQ_INT(search_case.completed_groups[i].smaller,
                      search_case.groups[i].smaller);
        ASSERT_EQ_INT(search_case.completed_groups[i].operation,
                      search_case.groups[i].operation);
        ASSERT_EQ_INT(search_case.completed_groups[i].greater,
                      search_case.groups[i].greater);
    }
}

static void test_find_answer_completion_fills_groups_around_a_full_strip(void)
{
    SearchCase search_case;
    start_save_case(&search_case, DIFFICULTY_HARD);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        search_case.strip[i] = search_case.puzzle.solution_array[i];
    }
    poison_outputs(&search_case);
    ASSERT_TRUE(run_search(&search_case));
    ASSERT_TRUE(completion_keeps_entries(&search_case));
    ASSERT_TRUE(completion_is_correct(&search_case));
}

static void test_find_answer_completion_accepts_other_valid_solution(void)
{
    SearchCase search_case;
    start_save_case(&search_case, DIFFICULTY_EASY);
    /* Cells 0 and 12 hold the pair (1,3) and cells 1 and 13 the pair (1,2),
     * swapping the generator's own pairing of cells 0, 1 and 12, 13. */
    set_group(&search_case, 0, 1, GROUP_OPERATION_MULTIPLY, 3);
    set_group(&search_case, 13, 1, GROUP_OPERATION_ADD, 2);
    ASSERT_FALSE(search_case.puzzle.solution_groups[0].operation ==
                 GROUP_OPERATION_MULTIPLY);
    poison_outputs(&search_case);
    ASSERT_TRUE(run_search(&search_case));
    ASSERT_TRUE(completion_keeps_entries(&search_case));
    ASSERT_TRUE(completion_is_correct(&search_case));
    ASSERT_EQ_INT(search_case.completed_groups[1].smaller, 1);
    ASSERT_EQ_INT(search_case.completed_groups[1].operation,
                  GROUP_OPERATION_MULTIPLY);
    ASSERT_EQ_INT(search_case.completed_groups[1].greater, 2);
    ASSERT_EQ_INT(search_case.completed_groups[12].operation,
                  GROUP_OPERATION_ADD);
}

static void test_find_answer_completion_reports_dead_end(void)
{
    SearchCase search_case;
    start_save_case(&search_case, DIFFICULTY_EASY);
    /* 1 + 6 reaches 7, but no cell holds the product 6 together with the
     * ones and sixes the strip can offer. */
    set_group(&search_case, 2, 1, GROUP_OPERATION_ADD, 6);
    BoardConflicts conflicts;
    find_board_conflicts(&search_case.puzzle, search_case.strip,
                         search_case.groups, &conflicts);
    ASSERT_EQ_INT(conflicts.count, 0);
    expect_search_fails_untouched(&search_case);
}

static void test_find_answer_completion_dead_end_from_a_wrong_slot(void)
{
    SearchCase search_case;
    start_save_case(&search_case, DIFFICULTY_EASY);
    search_case.strip[1] = 2; /* sorted and in range, but the strip needs a 1 */
    BoardConflicts conflicts;
    find_board_conflicts(&search_case.puzzle, search_case.strip,
                         search_case.groups, &conflicts);
    ASSERT_EQ_INT(conflicts.count, 0);
    expect_search_fails_untouched(&search_case);
}

static void test_find_answer_completion_rejects_out_of_range_strip(void)
{
    const int bad_values[] = {TEST_OUT_OF_RANGE_HIGH,
                              TEST_OUT_OF_RANGE_HIGH + 1, TEST_OUT_OF_RANGE_LOW,
                              -TEST_BIG_GROUP_NUMBER, TEST_BIG_GROUP_NUMBER};
    for (size_t i = 0; i < sizeof bad_values / sizeof bad_values[0]; i++)
    {
        SearchCase search_case;
        start_save_case(&search_case, DIFFICULTY_EASY);
        search_case.strip[1] = bad_values[i];
        expect_search_fails_untouched(&search_case);
        start_save_case(&search_case, DIFFICULTY_EASY);
        search_case.strip[12] = bad_values[i];
        expect_search_fails_untouched(&search_case);
    }
}

static void test_find_answer_completion_rejects_unsorted_strip(void)
{
    SearchCase search_case;
    start_save_case(&search_case, DIFFICULTY_EASY);
    search_case.strip[5] = 1; /* slot 4 gives 3 */
    expect_search_fails_untouched(&search_case);
    start_save_case(&search_case, DIFFICULTY_EASY);
    search_case.strip[1] = 3; /* slot 2 gives 2 */
    expect_search_fails_untouched(&search_case);
    start_save_case(&search_case, DIFFICULTY_EASY);
    search_case.strip[8] = 7; /* slot 9 gives 6 */
    expect_search_fails_untouched(&search_case);
}

static void test_find_answer_completion_rejects_group_numbers_out_of_range(void)
{
    SearchCase search_case;
    start_save_case(&search_case, DIFFICULTY_EASY);
    set_group(&search_case, 0, TEST_BIG_GROUP_NUMBER, GROUP_OPERATION_NONE,
              TETONOR_EMPTY_ENTRY);
    expect_search_fails_untouched(&search_case);
    start_save_case(&search_case, DIFFICULTY_EASY);
    set_group(&search_case, 3, TETONOR_EMPTY_ENTRY, GROUP_OPERATION_NONE,
              TEST_OUT_OF_RANGE_HIGH);
    expect_search_fails_untouched(&search_case);
    start_save_case(&search_case, DIFFICULTY_EASY);
    set_group(&search_case, 3, TEST_OUT_OF_RANGE_LOW, GROUP_OPERATION_ADD,
              TETONOR_EMPTY_ENTRY);
    expect_search_fails_untouched(&search_case);
}

static void test_find_answer_completion_handles_zero_targets(void)
{
    const int hidden[] = {0, 5, 10, 15};
    SearchCase search_case;
    build_fixture_puzzle(&search_case.puzzle, hidden, 4);
    strip_from_puzzle(&search_case.puzzle, search_case.strip);
    clear_groups(search_case.groups);
    ASSERT_EQ_INT(search_case.puzzle.sum_matrix[3][3], 0);
    poison_outputs(&search_case);
    ASSERT_TRUE(run_search(&search_case));
    ASSERT_TRUE(completion_is_correct(&search_case));
    ASSERT_EQ_INT(search_case.completed_strip[0], 0);
    ASSERT_EQ_INT(search_case.completed_strip[15], TETONOR_MAX_VALUE);
}

static void test_find_answer_completion_keeps_a_typed_zero(void)
{
    const int hidden[] = {0, 15};
    SearchCase search_case;
    build_fixture_puzzle(&search_case.puzzle, hidden, 2);
    strip_from_puzzle(&search_case.puzzle, search_case.strip);
    clear_groups(search_case.groups);
    search_case.strip[0] = 0;
    set_group(&search_case, 15, 0, GROUP_OPERATION_MULTIPLY,
              TETONOR_EMPTY_ENTRY);
    poison_outputs(&search_case);
    ASSERT_TRUE(run_search(&search_case));
    ASSERT_TRUE(completion_keeps_entries(&search_case));
    ASSERT_TRUE(completion_is_correct(&search_case));
}

static void test_find_answer_completion_all_given_strip_still_finds_groups(void)
{
    SearchCase search_case;
    build_fixture_puzzle(&search_case.puzzle, NULL, 0);
    strip_from_puzzle(&search_case.puzzle, search_case.strip);
    clear_groups(search_case.groups);
    poison_outputs(&search_case);
    ASSERT_TRUE(run_search(&search_case));
    ASSERT_TRUE(completion_is_correct(&search_case));
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        ASSERT_EQ_INT(search_case.completed_strip[i], fixture_strip[i]);
    }
}

static void test_find_answer_completion_is_deterministic(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
        {
            srand(seed);
            NumberGeneration* generated =
                generate_random_numbers(test_levels[level]);
            ASSERT_NOT_NULL(generated);
            SearchCase first;
            first.puzzle = *generated;
            free_number_generation(generated);
            strip_from_puzzle(&first.puzzle, first.strip);
            clear_groups(first.groups);
            reveal_part_of_the_solution(&first, seed);
            SearchCase second = first;
            poison_outputs(&first);
            poison_outputs(&second);
            ASSERT_TRUE(run_search(&first));
            ASSERT_TRUE(run_search(&second));
            ASSERT_EQ_INT(memcmp(first.completed_strip, second.completed_strip,
                                 sizeof first.completed_strip),
                          0);
            for (int i = 0; i < TETONOR_CELL_COUNT; i++)
            {
                ASSERT_EQ_INT(first.completed_groups[i].smaller,
                              second.completed_groups[i].smaller);
                ASSERT_EQ_INT(first.completed_groups[i].operation,
                              second.completed_groups[i].operation);
                ASSERT_EQ_INT(first.completed_groups[i].greater,
                              second.completed_groups[i].greater);
            }
        }
    }
}

static void test_find_answer_completion_does_not_read_the_solution_fields(void)
{
    for (int level = 0; level < TEST_LEVEL_COUNT; level++)
    {
        for (unsigned seed = 1; seed <= TEST_SEED_COUNT; seed++)
        {
            srand(seed);
            NumberGeneration* generated =
                generate_random_numbers(test_levels[level]);
            ASSERT_NOT_NULL(generated);
            SearchCase search_case;
            search_case.puzzle = *generated;
            free_number_generation(generated);
            memset(search_case.puzzle.solution_array, TEST_UNTOUCHED_BYTE,
                   sizeof search_case.puzzle.solution_array);
            memset(search_case.puzzle.solution_groups, TEST_UNTOUCHED_BYTE,
                   sizeof search_case.puzzle.solution_groups);
            strip_from_puzzle(&search_case.puzzle, search_case.strip);
            clear_groups(search_case.groups);
            ASSERT_TRUE(run_search(&search_case));
            ASSERT_TRUE(completion_is_correct(&search_case));
        }
    }
}

static void
test_find_answer_completion_gives_up_on_an_unsolvable_wide_search(void)
{
    SearchCase search_case;
    memset(&search_case.puzzle, 0, sizeof search_case.puzzle);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        int target = i < TETONOR_PAIR_COUNT ? TEST_WIDE_SUM_TARGET
                                            : TEST_WIDE_PRODUCT_TARGET;
        search_case.puzzle
            .sum_matrix[i / TETONOR_GRID_SIZE][i % TETONOR_GRID_SIZE] = target;
        search_case.puzzle.available_array[i] = wide_strip[i];
        search_case.strip[i]                  = wide_strip[i];
    }
    clear_groups(search_case.groups);
    expect_search_fails_untouched(&search_case);
}

int main(void)
{
    RUN_TEST(test_find_answer_completion_empty_board_is_correct);
    RUN_TEST(test_find_answer_completion_keeps_given_and_typed_entries);
    RUN_TEST(test_find_answer_completion_keeps_part_of_a_known_solution);
    RUN_TEST(test_find_answer_completion_returns_a_full_solution_unchanged);
    RUN_TEST(test_find_answer_completion_fills_groups_around_a_full_strip);
    RUN_TEST(test_find_answer_completion_accepts_other_valid_solution);
    RUN_TEST(test_find_answer_completion_reports_dead_end);
    RUN_TEST(test_find_answer_completion_dead_end_from_a_wrong_slot);
    RUN_TEST(test_find_answer_completion_rejects_out_of_range_strip);
    RUN_TEST(test_find_answer_completion_rejects_unsorted_strip);
    RUN_TEST(test_find_answer_completion_rejects_group_numbers_out_of_range);
    RUN_TEST(test_find_answer_completion_handles_zero_targets);
    RUN_TEST(test_find_answer_completion_keeps_a_typed_zero);
    RUN_TEST(test_find_answer_completion_all_given_strip_still_finds_groups);
    RUN_TEST(test_find_answer_completion_is_deterministic);
    RUN_TEST(test_find_answer_completion_does_not_read_the_solution_fields);
    RUN_TEST(test_find_answer_completion_gives_up_on_an_unsolvable_wide_search);
    return test_harness_result();
}
