#ifndef TETONOR_TESTS_GAME_FIXTURE_SAVED_GAME_H
#define TETONOR_TESTS_GAME_FIXTURE_SAVED_GAME_H

#include <stdbool.h>
#include <string.h>

#include "../../src/game/number_generation.h"
#include "../../src/game/operand_group.h"
#include "../../src/game/saved_game.h"

/* Fixture F-S: a puzzle whose values all lie in 1..12, so that one solution
 * is valid for every difficulty (the fixture F-A of fixture_puzzle.h holds 0
 * and 29, which no level allows). Eight pairs; cell 2k holds the sum and cell
 * 2k+1 the product of pair k. The strip is the sorted multiset of the pairs.
 * The first 4, 8 or 12 entries of save_fixture_hidden_order are hidden for
 * Easy, Medium and Hard, which are the counts the profiles require. */
static const int save_fixture_pairs[TETONOR_PAIR_COUNT][2] = {
    {1, 2}, {3, 4}, {5, 6}, {7, 8}, {9, 10}, {11, 12}, {1, 3}, {2, 4}};
static const int save_fixture_strip[TETONOR_CELL_COUNT] = {
    1, 1, 2, 2, 3, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12};
static const int save_fixture_hidden_order[TETONOR_CELL_COUNT - 4] = {
    1, 5, 8, 12, 0, 3, 10, 14, 2, 6, 9, 15};
static const int save_fixture_hidden_counts[DIFFICULTY_COUNT] = {4, 8, 12};

/* What save_fixture_hidden_order[0..3] hide, and the values typed in the
 * golden save. */
#define SAVE_FIXTURE_TYPED_SLOT 1
#define SAVE_FIXTURE_TYPED_VALUE 1
#define SAVE_FIXTURE_OTHER_TYPED_SLOT 8
#define SAVE_FIXTURE_OTHER_TYPED_VALUE 5
#define SAVE_FIXTURE_ELAPSED 754
#define SAVE_FIXTURE_MOVES 7

/* The lines of a save that follow the "moves" line (and, in version 2, the
 * "hint-used" line). */
#define SAVE_FIXTURE_BODY                                                      \
    "result pending\n"                                                         \
    "matrix 3 2 7 12 11 30 15 56 19 90 23 132 4 3 6 8\n"                       \
    "strip 1 - 2 2 3 - 4 4 - 6 7 8 - 10 11 12\n"                               \
    "solution 1 1 2 2 3 3 4 4 5 6 7 8 9 10 11 12\n"                            \
    "solution-groups 1+2 1x2 3+4 3x4 5+6 5x6 7+8 7x8 9+10 9x10 11+12 11x12 "   \
    "1+3 1x3 2+4 2x4\n"                                                        \
    "slots - 1 - - - - - - 5 - - - - - - -\n"                                  \
    "groups 1+2 -x- 12-- --- --- --- --- --- --- --- --- --- --- --- --- "     \
    "---\n"

/* serialize_saved_game of build_saved_game_fixture, byte for byte. */
#define SAVE_FIXTURE_TEXT                                                      \
    "TETONOR-SAVE 2\n"                                                         \
    "difficulty 0\n"                                                           \
    "elapsed 754\n"                                                            \
    "moves 7\n"                                                                \
    "hint-used 0\n" SAVE_FIXTURE_BODY

/* The same game in the legacy version 1 format, which has no "hint-used"
 * line. */
#define SAVE_FIXTURE_TEXT_V1                                                   \
    "TETONOR-SAVE 1\n"                                                         \
    "difficulty 0\n"                                                           \
    "elapsed 754\n"                                                            \
    "moves 7\n" SAVE_FIXTURE_BODY

static inline void build_save_fixture_puzzle(NumberGeneration* puzzle,
                                             int difficulty)
{
    memset(puzzle, 0, sizeof *puzzle);
    for (int pair = 0; pair < TETONOR_PAIR_COUNT; pair++)
    {
        int smaller  = save_fixture_pairs[pair][0];
        int greater  = save_fixture_pairs[pair][1];
        int sum_cell = 2 * pair;
        int product  = 2 * pair + 1;
        puzzle->sum_matrix[sum_cell / TETONOR_GRID_SIZE]
                          [sum_cell % TETONOR_GRID_SIZE] = smaller + greater;
        puzzle->sum_matrix[product / TETONOR_GRID_SIZE]
                          [product % TETONOR_GRID_SIZE] = smaller * greater;
        puzzle->solution_groups[sum_cell] =
            (OperandGroup){smaller, GROUP_OPERATION_ADD, greater};
        puzzle->solution_groups[product] =
            (OperandGroup){smaller, GROUP_OPERATION_MULTIPLY, greater};
    }
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        puzzle->solution_array[i]  = save_fixture_strip[i];
        puzzle->available_array[i] = save_fixture_strip[i];
    }
    for (int i = 0; i < save_fixture_hidden_counts[difficulty]; i++)
    {
        puzzle->available_array[save_fixture_hidden_order[i]] =
            TETONOR_HIDDEN_NUMBER;
    }
}

/* A valid save of the F-S puzzle with nothing typed. */
static inline void build_empty_saved_game(SavedGame* saved, int difficulty)
{
    memset(saved, 0, sizeof *saved);
    saved->difficulty      = difficulty;
    saved->elapsed_seconds = 0;
    saved->move_count      = 0;
    build_save_fixture_puzzle(&saved->puzzle, difficulty);
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        saved->entries.slot_values[i] = TETONOR_EMPTY_ENTRY;
        clear_operand_group(&saved->entries.groups[i]);
    }
    saved->entries.result = BOARD_RESULT_PENDING;
}

/* The save that SAVE_FIXTURE_TEXT describes: Easy, two slots typed and three
 * group boxes touched (seven boxes, seven moves). */
static inline void build_saved_game_fixture(SavedGame* saved)
{
    build_empty_saved_game(saved, DIFFICULTY_EASY);
    saved->elapsed_seconds = SAVE_FIXTURE_ELAPSED;
    saved->move_count      = SAVE_FIXTURE_MOVES;
    saved->entries.slot_values[SAVE_FIXTURE_TYPED_SLOT] =
        SAVE_FIXTURE_TYPED_VALUE;
    saved->entries.slot_values[SAVE_FIXTURE_OTHER_TYPED_SLOT] =
        SAVE_FIXTURE_OTHER_TYPED_VALUE;
    saved->entries.groups[0] = (OperandGroup){1, GROUP_OPERATION_ADD, 2};
    saved->entries.groups[1] = (OperandGroup){
        TETONOR_EMPTY_ENTRY, GROUP_OPERATION_MULTIPLY, TETONOR_EMPTY_ENTRY};
    saved->entries.groups[2] =
        (OperandGroup){12, GROUP_OPERATION_NONE, TETONOR_EMPTY_ENTRY};
}

/* Boxes holding something: typed slots, group numbers and operations. */
static inline int count_saved_game_filled_boxes(const SavedGame* saved)
{
    int filled = 0;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        const OperandGroup* group = &saved->entries.groups[i];
        filled += saved->entries.slot_values[i] != TETONOR_EMPTY_ENTRY;
        filled += group->smaller != TETONOR_EMPTY_ENTRY;
        filled += group->greater != TETONOR_EMPTY_ENTRY;
        filled += group->operation != GROUP_OPERATION_NONE;
    }
    return filled;
}

/* Every slot typed with the solution and every group box filled with the
 * generator's own groups, not yet submitted. move_count is the filled boxes,
 * the least a real game could have. */
static inline void fill_saved_game_with_solution(SavedGame* saved)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (saved->puzzle.available_array[i] == TETONOR_HIDDEN_NUMBER)
        {
            saved->entries.slot_values[i] = saved->puzzle.solution_array[i];
        }
        saved->entries.groups[i] = saved->puzzle.solution_groups[i];
    }
    saved->entries.result = BOARD_RESULT_PENDING;
    saved->move_count     = count_saved_game_filled_boxes(saved);
}

/* Fully filled and judged wrong: the first group has the wrong operation. */
static inline void fill_saved_game_with_wrong_answer(SavedGame* saved)
{
    fill_saved_game_with_solution(saved);
    saved->entries.groups[0].operation = GROUP_OPERATION_MULTIPLY;
    saved->entries.result              = BOARD_RESULT_WRONG;
}

static inline bool operand_groups_equal(const OperandGroup* first,
                                        const OperandGroup* second)
{
    return first->smaller == second->smaller &&
           first->operation == second->operation &&
           first->greater == second->greater;
}

static inline bool number_generations_equal(const NumberGeneration* first,
                                            const NumberGeneration* second)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (first->sum_matrix[i / TETONOR_GRID_SIZE][i % TETONOR_GRID_SIZE] !=
                second->sum_matrix[i / TETONOR_GRID_SIZE]
                                  [i % TETONOR_GRID_SIZE] ||
            first->available_array[i] != second->available_array[i] ||
            first->solution_array[i] != second->solution_array[i] ||
            !operand_groups_equal(&first->solution_groups[i],
                                  &second->solution_groups[i]))
        {
            return false;
        }
    }
    return true;
}

static inline bool snapshots_equal(const BoardSnapshot* first,
                                   const BoardSnapshot* second)
{
    if (first->result != second->result)
    {
        return false;
    }
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (first->slot_values[i] != second->slot_values[i] ||
            !operand_groups_equal(&first->groups[i], &second->groups[i]))
        {
            return false;
        }
    }
    return true;
}

/* Field by field: the padding bytes of a SavedGame are unspecified. */
static inline bool saved_games_equal(const SavedGame* first,
                                     const SavedGame* second)
{
    return first->difficulty == second->difficulty &&
           first->elapsed_seconds == second->elapsed_seconds &&
           first->move_count == second->move_count &&
           number_generations_equal(&first->puzzle, &second->puzzle) &&
           snapshots_equal(&first->entries, &second->entries);
}

#endif
