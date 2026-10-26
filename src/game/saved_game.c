#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "saved_game.h"
#include "answer_check.h"
#include "text_format.h"

#define SAVED_GAME_MAX_DIGITS 10 /* INT_MAX has ten digits */
#define SAVED_GAME_EMPTY_TOKEN "-"
#define SAVED_GAME_ADD_TOKEN "+"
#define SAVED_GAME_MULTIPLY_TOKEN "x"

/* The only format version; any other header is rejected (no migration). */
static const char saved_game_header[]   = "TETONOR-SAVE 1";

static const char key_difficulty[]      = "difficulty";
static const char key_elapsed[]         = "elapsed";
static const char key_moves[]           = "moves";
static const char key_result[]          = "result";
static const char key_matrix[]          = "matrix";
static const char key_strip[]           = "strip";
static const char key_solution[]        = "solution";
static const char key_solution_groups[] = "solution-groups";
static const char key_slots[]           = "slots";
static const char key_groups[]          = "groups";

static const char result_word_pending[] = "pending";
static const char result_word_wrong[]   = "wrong";

/* How a list token spells "nothing here", and the value it stands for. */
typedef struct
{
    bool allows_empty;
    int empty_value;
} EmptyToken;

static const EmptyToken no_empty_token    = {false, 0};
static const EmptyToken hidden_token      = {true, TETONOR_HIDDEN_NUMBER};
static const EmptyToken empty_entry_token = {true, TETONOR_EMPTY_ENTRY};

/* ------------------------------------------------------------ validation */

static bool is_in_range(int value, int first, int last)
{
    return value >= first && value <= last;
}

/* A box a player may have left empty or typed a number into. */
static bool is_entry_valid(int value)
{
    return value == TETONOR_EMPTY_ENTRY ||
           is_in_range(value, TETONOR_MIN_VALUE, TETONOR_MAX_VALUE);
}

static bool is_operation_valid(GroupOperation operation)
{
    return operation == GROUP_OPERATION_NONE ||
           operation == GROUP_OPERATION_ADD ||
           operation == GROUP_OPERATION_MULTIPLY;
}

/* V4: the generator's strip lies in the level's range and is sorted. */
static bool is_solution_strip_valid(const NumberGeneration* puzzle,
                                    const DifficultyProfile* profile)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (!is_in_range(puzzle->solution_array[i], profile->min_value,
                         profile->max_value) ||
            (i > 0 &&
             puzzle->solution_array[i] < puzzle->solution_array[i - 1]))
        {
            return false;
        }
    }
    return true;
}

/* V5: the masked strip hides exactly what the level hides, and shows the
 * solution's values everywhere else. */
static bool is_masked_strip_valid(const NumberGeneration* puzzle,
                                  const DifficultyProfile* profile)
{
    int hidden = 0;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (puzzle->available_array[i] == TETONOR_HIDDEN_NUMBER)
        {
            hidden++;
        }
        else if (puzzle->available_array[i] != puzzle->solution_array[i])
        {
            return false;
        }
    }
    return hidden ==
           2 * profile->fully_hidden_pairs + profile->half_hidden_pairs;
}

/* V4..V6: a puzzle shaped for the level that its stored solution solves. */
static bool is_puzzle_valid(const NumberGeneration* puzzle, int difficulty)
{
    DifficultyProfile profile = get_difficulty_profile(difficulty);
    return is_solution_strip_valid(puzzle, &profile) &&
           is_masked_strip_valid(puzzle, &profile) &&
           is_answer_correct(puzzle, puzzle->solution_array,
                             puzzle->solution_groups);
}

/* V7 and V8: only hidden slots hold an entry, and every box is in range. */
static bool are_entries_valid(const NumberGeneration* puzzle,
                              const BoardSnapshot* entries)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        const OperandGroup* group = &entries->groups[i];
        bool hidden = puzzle->available_array[i] == TETONOR_HIDDEN_NUMBER;
        if (!is_entry_valid(entries->slot_values[i]) ||
            (!hidden && entries->slot_values[i] != TETONOR_EMPTY_ENTRY) ||
            !is_entry_valid(group->smaller) ||
            !is_entry_valid(group->greater) ||
            !is_operation_valid(group->operation))
        {
            return false;
        }
    }
    return true;
}

static bool are_all_boxes_filled(const NumberGeneration* puzzle,
                                 const BoardSnapshot* entries)
{
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        bool hidden = puzzle->available_array[i] == TETONOR_HIDDEN_NUMBER;
        if ((hidden && entries->slot_values[i] == TETONOR_EMPTY_ENTRY) ||
            !is_operand_group_filled(&entries->groups[i]))
        {
            return false;
        }
    }
    return true;
}

/* The strip as the player sees it: given numbers plus typed ones. */
static bool is_answer_in_entries_correct(const NumberGeneration* puzzle,
                                         const BoardSnapshot* entries)
{
    int strip[TETONOR_CELL_COUNT];
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        strip[i] = puzzle->available_array[i] != TETONOR_HIDDEN_NUMBER
                       ? puzzle->available_array[i]
                       : entries->slot_values[i];
    }
    return is_answer_correct(puzzle, strip, entries->groups);
}

/* V9: only a pending or a judged-wrong board can be saved. */
static bool is_result_valid(const NumberGeneration* puzzle,
                            const BoardSnapshot* entries)
{
    if (entries->result == BOARD_RESULT_PENDING)
    {
        return true;
    }
    return entries->result == BOARD_RESULT_WRONG &&
           are_all_boxes_filled(puzzle, entries) &&
           !is_answer_in_entries_correct(puzzle, entries);
}

static int count_filled_boxes(const BoardSnapshot* entries)
{
    int filled = 0;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        const OperandGroup* group = &entries->groups[i];
        filled += entries->slot_values[i] != TETONOR_EMPTY_ENTRY;
        filled += group->smaller != TETONOR_EMPTY_ENTRY;
        filled += group->greater != TETONOR_EMPTY_ENTRY;
        filled += group->operation != GROUP_OPERATION_NONE;
    }
    return filled;
}

bool is_saved_game_valid(const SavedGame* saved)
{
    return is_in_range(saved->difficulty, DIFFICULTY_EASY, DIFFICULTY_HARD) &&
           is_in_range(saved->elapsed_seconds, 0, TETONOR_TIMER_MAX_SECONDS) &&
           saved->move_count >= 0 &&
           is_puzzle_valid(&saved->puzzle, saved->difficulty) &&
           are_entries_valid(&saved->puzzle, &saved->entries) &&
           is_result_valid(&saved->puzzle, &saved->entries) &&
           count_filled_boxes(&saved->entries) <= saved->move_count;
}

/* --------------------------------------------------------------- capture */

bool capture_saved_game(const Board* board, int32_t elapsed_seconds,
                        int difficulty, SavedGame* out)
{
    if (board->puzzle == NULL || is_board_locked(board))
    {
        return false;
    }
    SavedGame captured;
    memset(&captured, 0, sizeof captured);
    captured.difficulty      = difficulty;
    captured.elapsed_seconds = elapsed_seconds;
    captured.move_count      = board_move_count(board);
    captured.puzzle          = *board->puzzle;
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        captured.entries.slot_values[i] = board->slots[i].value;
        captured.entries.groups[i]      = board->groups[i];
    }
    captured.entries.result = board->result;
    if (!is_saved_game_valid(&captured))
    {
        return false;
    }
    *out = captured;
    return true;
}

/* ------------------------------------------------------------- serialize */

typedef struct
{
    char* text;
    size_t size;
    size_t used;
} OutputBuffer;

static bool put_text(OutputBuffer* buffer, const char* text)
{
    int written = snprintf(buffer->text + buffer->used,
                           buffer->size - buffer->used, "%s", text);
    return advance_text_output(written, buffer->size, &buffer->used);
}

static bool put_int(OutputBuffer* buffer, int value)
{
    int written = snprintf(buffer->text + buffer->used,
                           buffer->size - buffer->used, "%d", value);
    return advance_text_output(written, buffer->size, &buffer->used);
}

static bool put_token(OutputBuffer* buffer, int value, EmptyToken empty)
{
    if (empty.allows_empty && value == empty.empty_value)
    {
        return put_text(buffer, SAVED_GAME_EMPTY_TOKEN);
    }
    return put_int(buffer, value);
}

static bool put_operation(OutputBuffer* buffer, GroupOperation operation)
{
    switch (operation)
    {
        case GROUP_OPERATION_ADD:
            return put_text(buffer, SAVED_GAME_ADD_TOKEN);
        case GROUP_OPERATION_MULTIPLY:
            return put_text(buffer, SAVED_GAME_MULTIPLY_TOKEN);
        case GROUP_OPERATION_NONE:
            break;
    }
    return put_text(buffer, SAVED_GAME_EMPTY_TOKEN);
}

static bool put_group(OutputBuffer* buffer, const OperandGroup* group)
{
    return put_token(buffer, group->smaller, empty_entry_token) &&
           put_operation(buffer, group->operation) &&
           put_token(buffer, group->greater, empty_entry_token);
}

/* "<key> <value>\n" */
static bool put_scalar_line(OutputBuffer* buffer, const char* key, int value)
{
    return put_text(buffer, key) && put_text(buffer, " ") &&
           put_int(buffer, value) && put_text(buffer, "\n");
}

static bool put_result_line(OutputBuffer* buffer, BoardResult result)
{
    return put_text(buffer, key_result) && put_text(buffer, " ") &&
           put_text(buffer, result == BOARD_RESULT_WRONG
                                ? result_word_wrong
                                : result_word_pending) &&
           put_text(buffer, "\n");
}

/* "<key> <token> <token> ...\n" over one cell-sized list. */
static bool put_number_line(OutputBuffer* buffer, const char* key,
                            const int values[TETONOR_CELL_COUNT],
                            EmptyToken empty)
{
    if (!put_text(buffer, key))
    {
        return false;
    }
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (!put_text(buffer, " ") || !put_token(buffer, values[i], empty))
        {
            return false;
        }
    }
    return put_text(buffer, "\n");
}

static bool put_group_line(OutputBuffer* buffer, const char* key,
                           const OperandGroup groups[TETONOR_CELL_COUNT])
{
    if (!put_text(buffer, key))
    {
        return false;
    }
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (!put_text(buffer, " ") || !put_group(buffer, &groups[i]))
        {
            return false;
        }
    }
    return put_text(buffer, "\n");
}

static bool put_status_lines(OutputBuffer* buffer, const SavedGame* saved)
{
    return put_text(buffer, saved_game_header) && put_text(buffer, "\n") &&
           put_scalar_line(buffer, key_difficulty, saved->difficulty) &&
           put_scalar_line(buffer, key_elapsed, (int)saved->elapsed_seconds) &&
           put_scalar_line(buffer, key_moves, saved->move_count) &&
           put_result_line(buffer, saved->entries.result);
}

static bool put_board_lines(OutputBuffer* buffer, const SavedGame* saved)
{
    int matrix[TETONOR_CELL_COUNT];
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        matrix[i] =
            saved->puzzle
                .sum_matrix[i / TETONOR_GRID_SIZE][i % TETONOR_GRID_SIZE];
    }
    return put_number_line(buffer, key_matrix, matrix, no_empty_token) &&
           put_number_line(buffer, key_strip, saved->puzzle.available_array,
                           hidden_token) &&
           put_number_line(buffer, key_solution, saved->puzzle.solution_array,
                           no_empty_token) &&
           put_group_line(buffer, key_solution_groups,
                          saved->puzzle.solution_groups) &&
           put_number_line(buffer, key_slots, saved->entries.slot_values,
                           empty_entry_token) &&
           put_group_line(buffer, key_groups, saved->entries.groups);
}

bool serialize_saved_game(const SavedGame* saved, char* out, size_t size)
{
    OutputBuffer buffer = {out, size, 0};
    return put_status_lines(&buffer, saved) && put_board_lines(&buffer, saved);
}

/* ----------------------------------------------------------------- parse */

/* A line ends with a line end; only the last may end with the text. */
static bool read_line_end(TextCursor* cursor, bool last)
{
    return read_text_line_end(cursor) ||
           (last && is_text_cursor_at_end(cursor));
}

/* 1..SAVED_GAME_MAX_DIGITS digits that fit an int. Read as 64 bits first, so
 * a number past INT_MAX is rejected rather than wrapped. */
static bool read_int_value(TextCursor* cursor, int max, int* value)
{
    int64_t number;
    if (!read_text_number(cursor, SAVED_GAME_MAX_DIGITS, &number) ||
        number > max)
    {
        return false;
    }
    *value = (int)number;
    return true;
}

static bool read_token(TextCursor* cursor, EmptyToken empty, int* value)
{
    if (empty.allows_empty && read_text_literal(cursor, SAVED_GAME_EMPTY_TOKEN))
    {
        *value = empty.empty_value;
        return true;
    }
    return read_int_value(cursor, INT_MAX, value);
}

static bool read_operation(TextCursor* cursor, GroupOperation* operation)
{
    if (read_text_literal(cursor, SAVED_GAME_ADD_TOKEN))
    {
        *operation = GROUP_OPERATION_ADD;
        return true;
    }
    if (read_text_literal(cursor, SAVED_GAME_MULTIPLY_TOKEN))
    {
        *operation = GROUP_OPERATION_MULTIPLY;
        return true;
    }
    if (read_text_literal(cursor, SAVED_GAME_EMPTY_TOKEN))
    {
        *operation = GROUP_OPERATION_NONE;
        return true;
    }
    return false;
}

static bool read_group(TextCursor* cursor, OperandGroup* group)
{
    return read_token(cursor, empty_entry_token, &group->smaller) &&
           read_operation(cursor, &group->operation) &&
           read_token(cursor, empty_entry_token, &group->greater);
}

/* "<key> <value>" without its line end. */
static bool read_scalar(TextCursor* cursor, const char* key, int max,
                        int* value)
{
    return read_text_literal(cursor, key) && read_text_literal(cursor, " ") &&
           read_int_value(cursor, max, value);
}

static bool read_result(TextCursor* cursor, BoardResult* result)
{
    if (!read_text_literal(cursor, key_result) ||
        !read_text_literal(cursor, " "))
    {
        return false;
    }
    if (read_text_literal(cursor, result_word_pending))
    {
        *result = BOARD_RESULT_PENDING;
        return true;
    }
    if (read_text_literal(cursor, result_word_wrong))
    {
        *result = BOARD_RESULT_WRONG;
        return true;
    }
    return false;
}

/* "<key> <token> <token> ..." over one cell-sized list, without its line end.
 */
static bool read_number_list(TextCursor* cursor, const char* key,
                             EmptyToken empty, int values[TETONOR_CELL_COUNT])
{
    if (!read_text_literal(cursor, key))
    {
        return false;
    }
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (!read_text_literal(cursor, " ") ||
            !read_token(cursor, empty, &values[i]))
        {
            return false;
        }
    }
    return true;
}

static bool read_group_list(TextCursor* cursor, const char* key,
                            OperandGroup groups[TETONOR_CELL_COUNT])
{
    if (!read_text_literal(cursor, key))
    {
        return false;
    }
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (!read_text_literal(cursor, " ") || !read_group(cursor, &groups[i]))
        {
            return false;
        }
    }
    return true;
}

static bool read_status_lines(TextCursor* cursor, SavedGame* saved)
{
    int elapsed = 0;
    if (!read_text_literal(cursor, saved_game_header) ||
        !read_line_end(cursor, false) ||
        !read_scalar(cursor, key_difficulty, DIFFICULTY_HARD,
                     &saved->difficulty) ||
        !read_line_end(cursor, false) ||
        !read_scalar(cursor, key_elapsed, TETONOR_TIMER_MAX_SECONDS,
                     &elapsed) ||
        !read_line_end(cursor, false) ||
        !read_scalar(cursor, key_moves, INT_MAX, &saved->move_count) ||
        !read_line_end(cursor, false) ||
        !read_result(cursor, &saved->entries.result) ||
        !read_line_end(cursor, false))
    {
        return false;
    }
    saved->elapsed_seconds = elapsed;
    return true;
}

static bool read_board_lines(TextCursor* cursor, SavedGame* saved)
{
    int matrix[TETONOR_CELL_COUNT];
    if (!read_number_list(cursor, key_matrix, no_empty_token, matrix) ||
        !read_line_end(cursor, false) ||
        !read_number_list(cursor, key_strip, hidden_token,
                          saved->puzzle.available_array) ||
        !read_line_end(cursor, false) ||
        !read_number_list(cursor, key_solution, no_empty_token,
                          saved->puzzle.solution_array) ||
        !read_line_end(cursor, false) ||
        !read_group_list(cursor, key_solution_groups,
                         saved->puzzle.solution_groups) ||
        !read_line_end(cursor, false) ||
        !read_number_list(cursor, key_slots, empty_entry_token,
                          saved->entries.slot_values) ||
        !read_line_end(cursor, false) ||
        !read_group_list(cursor, key_groups, saved->entries.groups) ||
        !read_line_end(cursor, true))
    {
        return false;
    }
    for (int i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        saved->puzzle.sum_matrix[i / TETONOR_GRID_SIZE][i % TETONOR_GRID_SIZE] =
            matrix[i];
    }
    return true;
}

bool parse_saved_game(const char* text, size_t length, SavedGame* saved)
{
    if (length > TETONOR_SAVED_GAME_TEXT_MAX)
    {
        return false;
    }
    TextCursor cursor = make_text_cursor(text, length);
    SavedGame parsed;
    memset(&parsed, 0, sizeof parsed);
    if (!read_status_lines(&cursor, &parsed) ||
        !read_board_lines(&cursor, &parsed) ||
        !is_text_cursor_at_end(&cursor) || !is_saved_game_valid(&parsed))
    {
        return false;
    }
    *saved = parsed;
    return true;
}

/* --------------------------------------------------------------- restore */

NumberGeneration* restore_saved_game(const SavedGame* saved, Board* board,
                                     GameTimer* timer, time_t now)
{
    if (!is_saved_game_valid(saved))
    {
        return NULL;
    }
    NumberGeneration* puzzle = alloc_number_generation_copy(&saved->puzzle);
    if (puzzle == NULL)
    {
        return NULL;
    }
    restore_board(board, puzzle, &saved->entries, saved->move_count);
    restore_game_timer(timer, saved->elapsed_seconds, now);
    return puzzle;
}
