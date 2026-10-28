#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../test_harness.h"
#include "../game/fixture_saved_game.h"
#include "../../src/storage/saved_game_store.h"
#include "../../src/storage/stats_store.h"

/* make test recreates this directory before running any test binary. */
#define TEST_TMP_DIR "build/tests/tmp"
#define TEST_PATH(name) (TEST_TMP_DIR "/" name)
#define TEST_TEXT_BUFFER_SIZE 2048
#define TEST_UNTOUCHED_BYTE 0x5a
#define TEST_OTHER_ELAPSED 100
#define TEST_TEMPORARY_SUFFIX ".tmp"

static void build_other_saved_game(SavedGame* saved)
{
    build_empty_saved_game(saved, DIFFICULTY_MEDIUM);
    saved->elapsed_seconds                              = TEST_OTHER_ELAPSED;
    saved->entries.slot_values[SAVE_FIXTURE_TYPED_SLOT] = 1;
    saved->move_count                                   = 1;
}

static void build_invalid_saved_game(SavedGame* saved)
{
    build_saved_game_fixture(saved);
    saved->elapsed_seconds = -1;
}

static bool file_exists(const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL)
    {
        return false;
    }
    fclose(file);
    return true;
}

static bool write_file(const char* path, const void* bytes, size_t length)
{
    FILE* file = fopen(path, "wb");
    if (file == NULL)
    {
        return false;
    }
    size_t written = fwrite(bytes, 1, length, file);
    return fclose(file) == 0 && written == length;
}

static bool write_text_file(const char* path, const char* text)
{
    return write_file(path, text, strlen(text));
}

static size_t read_file(const char* path, char* out, size_t size)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL)
    {
        return 0;
    }
    size_t length = fread(out, 1, size, file);
    fclose(file);
    return length;
}

static bool file_holds(const char* path, const char* expected)
{
    char text[TEST_TEXT_BUFFER_SIZE];
    size_t length = read_file(path, text, sizeof text);
    return length == strlen(expected) && memcmp(text, expected, length) == 0;
}

static void remove_with_temporary(const char* path)
{
    char temporary[TETONOR_SAVED_GAME_PATH_MAX + sizeof TEST_TEMPORARY_SUFFIX];
    remove(path);
    snprintf(temporary, sizeof temporary, "%s" TEST_TEMPORARY_SUFFIX, path);
    remove(temporary);
}

static bool temporary_file_exists(const char* path)
{
    char temporary[TETONOR_SAVED_GAME_PATH_MAX + sizeof TEST_TEMPORARY_SUFFIX];
    snprintf(temporary, sizeof temporary, "%s" TEST_TEMPORARY_SUFFIX, path);
    return file_exists(temporary);
}

static void test_saved_game_path_max_shares_the_text_file_limit(void)
{
    ASSERT_EQ_INT(TETONOR_SAVED_GAME_PATH_MAX, TETONOR_TEXT_FILE_PATH_MAX);
}

static void test_save_and_load_saved_game_file_round_trip(void)
{
    const char* path = TEST_PATH("saved_round_trip.txt");
    remove_with_temporary(path);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    ASSERT_TRUE(save_saved_game_file(path, &saved));
    ASSERT_FALSE(temporary_file_exists(path));
    SavedGame loaded;
    build_other_saved_game(&loaded);
    ASSERT_EQ_INT(load_saved_game_file(path, &loaded), SAVED_GAME_LOAD_LOADED);
    remove_with_temporary(path);
    ASSERT_TRUE(saved_games_equal(&loaded, &saved));
}

static void test_save_saved_game_file_writes_the_documented_text(void)
{
    const char* path = TEST_PATH("saved_documented_text.txt");
    remove_with_temporary(path);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    ASSERT_TRUE(save_saved_game_file(path, &saved));
    bool same = file_holds(path, SAVE_FIXTURE_TEXT);
    remove_with_temporary(path);
    ASSERT_TRUE(same);
}

static void test_save_saved_game_file_overwrites_an_existing_file(void)
{
    const char* path = TEST_PATH("saved_overwrite.txt");
    remove_with_temporary(path);
    SavedGame first;
    SavedGame second;
    build_saved_game_fixture(&first);
    build_other_saved_game(&second);
    ASSERT_TRUE(save_saved_game_file(path, &first));
    ASSERT_TRUE(save_saved_game_file(path, &second));
    SavedGame loaded;
    ASSERT_EQ_INT(load_saved_game_file(path, &loaded), SAVED_GAME_LOAD_LOADED);
    remove_with_temporary(path);
    ASSERT_TRUE(saved_games_equal(&loaded, &second));
}

static void test_save_saved_game_file_replaces_a_corrupted_file(void)
{
    const char* path = TEST_PATH("saved_overwrite_corrupted.txt");
    remove_with_temporary(path);
    char garbage[TEST_TEXT_BUFFER_SIZE];
    memset(garbage, '\xff', sizeof garbage);
    ASSERT_TRUE(write_file(path, garbage, sizeof garbage));
    SavedGame saved;
    build_saved_game_fixture(&saved);
    ASSERT_TRUE(save_saved_game_file(path, &saved));
    bool same = file_holds(path, SAVE_FIXTURE_TEXT);
    remove_with_temporary(path);
    ASSERT_TRUE(same);
}

static void test_save_saved_game_file_unwritable_location_fails_cleanly(void)
{
    const char* path = TEST_PATH("no_such_directory/saved.txt");
    SavedGame saved;
    build_saved_game_fixture(&saved);
    ASSERT_FALSE(save_saved_game_file(path, &saved));
    ASSERT_FALSE(file_exists(path));
    ASSERT_FALSE(temporary_file_exists(path));
}

static void test_save_saved_game_file_rejects_too_long_path(void)
{
    char path[TETONOR_SAVED_GAME_PATH_MAX + 1];
    memset(path, 'a', sizeof path);
    path[TETONOR_SAVED_GAME_PATH_MAX] = '\0';
    SavedGame saved;
    build_saved_game_fixture(&saved);
    ASSERT_FALSE(save_saved_game_file(path, &saved));
}

static void expect_load_result(const char* path, SavedGameLoadResult expected)
{
    SavedGame game;
    memset(&game, TEST_UNTOUCHED_BYTE, sizeof game);
    unsigned char before[sizeof game];
    memcpy(before, &game, sizeof game);
    ASSERT_EQ_INT(load_saved_game_file(path, &game), expected);
    ASSERT_EQ_INT(memcmp(before, &game, sizeof game), 0);
}

static void test_load_saved_game_file_missing_file_is_missing(void)
{
    const char* path = TEST_PATH("saved_missing.txt");
    remove_with_temporary(path);
    expect_load_result(path, SAVED_GAME_LOAD_MISSING);
}

static void test_load_saved_game_file_empty_file_is_corrupted(void)
{
    const char* path = TEST_PATH("saved_empty.txt");
    ASSERT_TRUE(write_file(path, "", 0));
    expect_load_result(path, SAVED_GAME_LOAD_CORRUPTED);
    remove_with_temporary(path);
}

static void test_load_saved_game_file_header_only_is_corrupted(void)
{
    const char* path = TEST_PATH("saved_header_only.txt");
    ASSERT_TRUE(write_text_file(path, "TETONOR-SAVE 1\n"));
    expect_load_result(path, SAVED_GAME_LOAD_CORRUPTED);
    remove_with_temporary(path);
}

static void test_load_saved_game_file_garbage_is_corrupted(void)
{
    const char* path = TEST_PATH("saved_garbage.txt");
    char garbage[TEST_TEXT_BUFFER_SIZE / 4];
    for (size_t i = 0; i < sizeof garbage; i++)
    {
        garbage[i] = (char)(i * 7 + 3);
    }
    ASSERT_TRUE(write_file(path, garbage, sizeof garbage));
    expect_load_result(path, SAVED_GAME_LOAD_CORRUPTED);
    remove_with_temporary(path);
}

static void test_load_saved_game_file_truncated_file_is_corrupted(void)
{
    const char* path = TEST_PATH("saved_truncated.txt");
    size_t half      = strlen(SAVE_FIXTURE_TEXT) / 2;
    ASSERT_TRUE(write_file(path, SAVE_FIXTURE_TEXT, half));
    expect_load_result(path, SAVED_GAME_LOAD_CORRUPTED);
    ASSERT_TRUE(
        write_file(path, SAVE_FIXTURE_TEXT, strlen(SAVE_FIXTURE_TEXT) - 2));
    expect_load_result(path, SAVED_GAME_LOAD_CORRUPTED);
    remove_with_temporary(path);
}

static void test_load_saved_game_file_oversized_file_is_corrupted(void)
{
    const char* path = TEST_PATH("saved_oversized.txt");
    char text[TEST_TEXT_BUFFER_SIZE];
    size_t length = strlen(SAVE_FIXTURE_TEXT);
    memcpy(text, SAVE_FIXTURE_TEXT, length);
    memset(text + length, '\n', sizeof text - length);
    ASSERT_TRUE(write_file(path, text, TETONOR_SAVED_GAME_TEXT_MAX + 1));
    expect_load_result(path, SAVED_GAME_LOAD_CORRUPTED);
    ASSERT_TRUE(write_file(path, text, sizeof text));
    expect_load_result(path, SAVED_GAME_LOAD_CORRUPTED);
    remove_with_temporary(path);
}

static void test_load_saved_game_file_unsupported_version_is_corrupted(void)
{
    const char* path = TEST_PATH("saved_version.txt");
    char text[TEST_TEXT_BUFFER_SIZE];
    snprintf(text, sizeof text, "TETONOR-SAVE 3%s",
             strchr(SAVE_FIXTURE_TEXT, '\n'));
    ASSERT_TRUE(write_text_file(path, text));
    expect_load_result(path, SAVED_GAME_LOAD_CORRUPTED);
    remove_with_temporary(path);
}

static void test_load_saved_game_file_accepts_a_version_one_file(void)
{
    const char* path = TEST_PATH("saved_version_one.txt");
    ASSERT_TRUE(write_text_file(path, SAVE_FIXTURE_TEXT_V1));
    SavedGame expected;
    build_saved_game_fixture(&expected);
    SavedGame loaded;
    build_other_saved_game(&loaded);
    loaded.hint_used = true;
    ASSERT_EQ_INT(load_saved_game_file(path, &loaded), SAVED_GAME_LOAD_LOADED);
    remove_with_temporary(path);
    ASSERT_FALSE(loaded.hint_used);
    ASSERT_TRUE(saved_games_equal(&loaded, &expected));
}

static void test_save_saved_game_file_round_trips_the_hint_mark(void)
{
    const char* path = TEST_PATH("saved_hint_mark.txt");
    remove_with_temporary(path);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    saved.hint_used = true;
    ASSERT_TRUE(save_saved_game_file(path, &saved));
    SavedGame loaded;
    build_other_saved_game(&loaded);
    ASSERT_EQ_INT(load_saved_game_file(path, &loaded), SAVED_GAME_LOAD_LOADED);
    remove_with_temporary(path);
    ASSERT_TRUE(loaded.hint_used);
    ASSERT_TRUE(saved_games_equal(&loaded, &saved));
}

static void test_load_saved_game_file_inconsistent_content_is_corrupted(void)
{
    const char* path = TEST_PATH("saved_inconsistent.txt");
    ASSERT_TRUE(write_text_file(
        path, "TETONOR-SAVE 1\ndifficulty 0\nelapsed 360000\nmoves 7\n"));
    expect_load_result(path, SAVED_GAME_LOAD_CORRUPTED);
    char text[TEST_TEXT_BUFFER_SIZE];
    snprintf(text, sizeof text, "%s", SAVE_FIXTURE_TEXT);
    char* moves = strstr(text, "moves 7");
    ASSERT_NOT_NULL(moves);
    moves[strlen("moves ")] = '0';
    ASSERT_TRUE(write_text_file(path, text));
    expect_load_result(path, SAVED_GAME_LOAD_CORRUPTED);
    remove_with_temporary(path);
}

static void test_load_saved_game_file_accepts_a_crlf_file(void)
{
    const char* path = TEST_PATH("saved_crlf.txt");
    char text[TEST_TEXT_BUFFER_SIZE];
    size_t length = 0;
    for (const char* at = SAVE_FIXTURE_TEXT; *at != '\0'; at++)
    {
        if (*at == '\n')
        {
            text[length++] = '\r';
        }
        text[length++] = *at;
    }
    ASSERT_TRUE(write_file(path, text, length));
    SavedGame expected;
    build_saved_game_fixture(&expected);
    SavedGame loaded;
    SavedGameLoadResult result = load_saved_game_file(path, &loaded);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, SAVED_GAME_LOAD_LOADED);
    ASSERT_TRUE(saved_games_equal(&loaded, &expected));
}

static void test_load_saved_game_file_leaves_the_file_alone(void)
{
    const char* path = TEST_PATH("saved_load_readonly.txt");
    ASSERT_TRUE(write_text_file(path, "not a saved game"));
    SavedGame game;
    ASSERT_EQ_INT(load_saved_game_file(path, &game), SAVED_GAME_LOAD_CORRUPTED);
    bool same = file_holds(path, "not a saved game");
    remove_with_temporary(path);
    ASSERT_TRUE(same);
}

static void test_init_saved_game_store_without_path_is_memory_only(void)
{
    SavedGameStore store;
    memset(&store, TEST_UNTOUCHED_BYTE, sizeof store);
    ASSERT_EQ_INT(init_saved_game_store(&store, NULL),
                  SAVED_GAME_LOAD_DISABLED);
    ASSERT_FALSE(store.persistent);
    ASSERT_FALSE(store.available);
    ASSERT_NULL(saved_game_store_game(&store));
    memset(&store, TEST_UNTOUCHED_BYTE, sizeof store);
    ASSERT_EQ_INT(init_saved_game_store(&store, ""), SAVED_GAME_LOAD_DISABLED);
    ASSERT_FALSE(store.persistent);
    ASSERT_FALSE(store.available);
    ASSERT_NULL(saved_game_store_game(&store));
}

static void test_init_saved_game_store_too_long_path_is_memory_only(void)
{
    char path[TETONOR_SAVED_GAME_PATH_MAX + 1];
    memset(path, 'a', sizeof path);
    path[TETONOR_SAVED_GAME_PATH_MAX] = '\0';
    SavedGameStore store;
    ASSERT_EQ_INT(init_saved_game_store(&store, path),
                  SAVED_GAME_LOAD_DISABLED);
    ASSERT_FALSE(store.persistent);
    ASSERT_FALSE(store.available);
    path[TETONOR_SAVED_GAME_PATH_MAX - 1] = '\0';
    ASSERT_EQ_INT(init_saved_game_store(&store, path), SAVED_GAME_LOAD_MISSING);
    ASSERT_TRUE(store.persistent);
    ASSERT_FALSE(store.available);
}

static void test_init_saved_game_store_missing_file_has_no_game(void)
{
    const char* path = TEST_PATH("store_missing.txt");
    remove_with_temporary(path);
    SavedGameStore store;
    ASSERT_EQ_INT(init_saved_game_store(&store, path), SAVED_GAME_LOAD_MISSING);
    ASSERT_TRUE(store.persistent);
    ASSERT_TRUE(strcmp(store.path, path) == 0);
    ASSERT_FALSE(store.available);
    ASSERT_NULL(saved_game_store_game(&store));
    ASSERT_FALSE(file_exists(path));
}

static void test_init_saved_game_store_loads_an_existing_file(void)
{
    const char* path = TEST_PATH("store_existing.txt");
    ASSERT_TRUE(write_text_file(path, SAVE_FIXTURE_TEXT));
    SavedGameStore store;
    SavedGameLoadResult result = init_saved_game_store(&store, path);
    remove_with_temporary(path);
    SavedGame expected;
    build_saved_game_fixture(&expected);
    ASSERT_EQ_INT(result, SAVED_GAME_LOAD_LOADED);
    ASSERT_TRUE(store.persistent);
    ASSERT_TRUE(store.available);
    const SavedGame* game = saved_game_store_game(&store);
    ASSERT_NOT_NULL(game);
    ASSERT_TRUE(saved_games_equal(game, &expected));
}

static void test_init_saved_game_store_corrupted_file_has_no_game(void)
{
    const char* path = TEST_PATH("store_corrupted.txt");
    ASSERT_TRUE(write_text_file(path, "TETONOR-SAVE 1\nnonsense\n"));
    SavedGameStore store;
    SavedGameLoadResult result = init_saved_game_store(&store, path);
    bool untouched             = file_holds(path, "TETONOR-SAVE 1\nnonsense\n");
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, SAVED_GAME_LOAD_CORRUPTED);
    ASSERT_TRUE(store.persistent);
    ASSERT_FALSE(store.available);
    ASSERT_NULL(saved_game_store_game(&store));
    ASSERT_TRUE(untouched);
}

static void test_init_saved_game_store_replaces_a_previous_state(void)
{
    const char* path = TEST_PATH("store_reinit.txt");
    remove_with_temporary(path);
    SavedGameStore store;
    SavedGame saved;
    build_saved_game_fixture(&saved);
    init_saved_game_store(&store, NULL);
    ASSERT_EQ_INT(save_saved_game_store(&store, &saved),
                  SAVED_GAME_SAVE_SESSION_ONLY);
    ASSERT_TRUE(store.available);
    ASSERT_EQ_INT(init_saved_game_store(&store, path), SAVED_GAME_LOAD_MISSING);
    ASSERT_FALSE(store.available);
    ASSERT_NULL(saved_game_store_game(&store));
}

static void test_save_saved_game_store_without_path_stays_in_memory(void)
{
    SavedGameStore store;
    init_saved_game_store(&store, NULL);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    ASSERT_EQ_INT(save_saved_game_store(&store, &saved),
                  SAVED_GAME_SAVE_SESSION_ONLY);
    ASSERT_TRUE(store.available);
    ASSERT_FALSE(store.persistent);
    const SavedGame* game = saved_game_store_game(&store);
    ASSERT_NOT_NULL(game);
    ASSERT_TRUE(saved_games_equal(game, &saved));
}

static void test_save_saved_game_store_keeps_a_copy(void)
{
    SavedGameStore store;
    init_saved_game_store(&store, NULL);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    SavedGame expected = saved;
    ASSERT_EQ_INT(save_saved_game_store(&store, &saved),
                  SAVED_GAME_SAVE_SESSION_ONLY);
    saved.elapsed_seconds                              = TEST_OTHER_ELAPSED;
    saved.entries.slot_values[SAVE_FIXTURE_TYPED_SLOT] = TETONOR_EMPTY_ENTRY;
    ASSERT_TRUE(saved_games_equal(saved_game_store_game(&store), &expected));
}

static void test_save_saved_game_store_persists_for_the_next_run(void)
{
    const char* path = TEST_PATH("store_persist.txt");
    remove_with_temporary(path);
    SavedGameStore store;
    ASSERT_EQ_INT(init_saved_game_store(&store, path), SAVED_GAME_LOAD_MISSING);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    ASSERT_EQ_INT(save_saved_game_store(&store, &saved),
                  SAVED_GAME_SAVE_STORED);
    ASSERT_TRUE(store.available);
    ASSERT_TRUE(file_holds(path, SAVE_FIXTURE_TEXT));
    ASSERT_FALSE(temporary_file_exists(path));
    SavedGameStore next_run;
    SavedGameLoadResult result = init_saved_game_store(&next_run, path);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, SAVED_GAME_LOAD_LOADED);
    ASSERT_TRUE(next_run.available);
    ASSERT_TRUE(saved_games_equal(saved_game_store_game(&next_run), &saved));
}

static void test_save_saved_game_store_replaces_the_previous_save(void)
{
    const char* path = TEST_PATH("store_replace.txt");
    remove_with_temporary(path);
    SavedGameStore store;
    init_saved_game_store(&store, path);
    SavedGame first;
    SavedGame second;
    build_saved_game_fixture(&first);
    build_other_saved_game(&second);
    ASSERT_EQ_INT(save_saved_game_store(&store, &first),
                  SAVED_GAME_SAVE_STORED);
    ASSERT_EQ_INT(save_saved_game_store(&store, &second),
                  SAVED_GAME_SAVE_STORED);
    ASSERT_TRUE(saved_games_equal(saved_game_store_game(&store), &second));
    SavedGameStore next_run;
    SavedGameLoadResult result = init_saved_game_store(&next_run, path);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, SAVED_GAME_LOAD_LOADED);
    ASSERT_TRUE(saved_games_equal(saved_game_store_game(&next_run), &second));
}

static void test_save_saved_game_store_replaces_a_corrupted_file(void)
{
    const char* path = TEST_PATH("store_replace_corrupted.txt");
    ASSERT_TRUE(write_text_file(path, "damaged"));
    SavedGameStore store;
    ASSERT_EQ_INT(init_saved_game_store(&store, path),
                  SAVED_GAME_LOAD_CORRUPTED);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    ASSERT_EQ_INT(save_saved_game_store(&store, &saved),
                  SAVED_GAME_SAVE_STORED);
    SavedGameStore next_run;
    SavedGameLoadResult result = init_saved_game_store(&next_run, path);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, SAVED_GAME_LOAD_LOADED);
    ASSERT_TRUE(saved_games_equal(saved_game_store_game(&next_run), &saved));
}

static void test_save_saved_game_store_rejects_an_invalid_game(void)
{
    const char* path = TEST_PATH("store_reject.txt");
    remove_with_temporary(path);
    SavedGameStore store;
    init_saved_game_store(&store, path);
    SavedGame invalid;
    build_invalid_saved_game(&invalid);
    ASSERT_EQ_INT(save_saved_game_store(&store, &invalid),
                  SAVED_GAME_SAVE_REJECTED);
    ASSERT_FALSE(store.available);
    ASSERT_NULL(saved_game_store_game(&store));
    ASSERT_FALSE(file_exists(path));
    ASSERT_FALSE(temporary_file_exists(path));
}

static void test_save_saved_game_store_rejection_keeps_the_previous_save(void)
{
    const char* path = TEST_PATH("store_reject_keeps.txt");
    remove_with_temporary(path);
    SavedGameStore store;
    init_saved_game_store(&store, path);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    ASSERT_EQ_INT(save_saved_game_store(&store, &saved),
                  SAVED_GAME_SAVE_STORED);
    SavedGame invalid;
    build_invalid_saved_game(&invalid);
    ASSERT_EQ_INT(save_saved_game_store(&store, &invalid),
                  SAVED_GAME_SAVE_REJECTED);
    ASSERT_TRUE(store.available);
    ASSERT_TRUE(saved_games_equal(saved_game_store_game(&store), &saved));
    bool file_kept = file_holds(path, SAVE_FIXTURE_TEXT);
    remove_with_temporary(path);
    ASSERT_TRUE(file_kept);
}

static void test_save_saved_game_store_rejects_an_unsolvable_game(void)
{
    SavedGameStore store;
    init_saved_game_store(&store, NULL);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    saved.puzzle.sum_matrix[0][0]++;
    ASSERT_EQ_INT(save_saved_game_store(&store, &saved),
                  SAVED_GAME_SAVE_REJECTED);
    ASSERT_FALSE(store.available);
}

static void
test_save_saved_game_store_write_failure_keeps_the_game_in_memory(void)
{
    const char* path = TEST_PATH("no_such_directory/store.txt");
    SavedGameStore store;
    init_saved_game_store(&store, path);
    ASSERT_TRUE(store.persistent);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    ASSERT_EQ_INT(save_saved_game_store(&store, &saved),
                  SAVED_GAME_SAVE_SESSION_ONLY);
    ASSERT_TRUE(store.available);
    ASSERT_TRUE(saved_games_equal(saved_game_store_game(&store), &saved));
    ASSERT_FALSE(file_exists(path));
    ASSERT_FALSE(temporary_file_exists(path));
    SavedGame other;
    build_other_saved_game(&other);
    ASSERT_EQ_INT(save_saved_game_store(&store, &other),
                  SAVED_GAME_SAVE_SESSION_ONLY);
    ASSERT_TRUE(saved_games_equal(saved_game_store_game(&store), &other));
}

static void test_discard_saved_game_store_forgets_the_game_and_the_file(void)
{
    const char* path = TEST_PATH("store_discard.txt");
    remove_with_temporary(path);
    SavedGameStore store;
    init_saved_game_store(&store, path);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    ASSERT_EQ_INT(save_saved_game_store(&store, &saved),
                  SAVED_GAME_SAVE_STORED);
    discard_saved_game_store(&store);
    ASSERT_FALSE(store.available);
    ASSERT_NULL(saved_game_store_game(&store));
    ASSERT_FALSE(file_exists(path));
    ASSERT_TRUE(store.persistent);
    SavedGameStore next_run;
    ASSERT_EQ_INT(init_saved_game_store(&next_run, path),
                  SAVED_GAME_LOAD_MISSING);
}

static void test_discard_saved_game_store_without_a_game_is_harmless(void)
{
    const char* path = TEST_PATH("store_discard_nothing.txt");
    remove_with_temporary(path);
    SavedGameStore store;
    init_saved_game_store(&store, path);
    discard_saved_game_store(&store);
    discard_saved_game_store(&store);
    ASSERT_FALSE(store.available);
    ASSERT_TRUE(store.persistent);
    ASSERT_FALSE(file_exists(path));
}

static void test_discard_saved_game_store_memory_only(void)
{
    SavedGameStore store;
    init_saved_game_store(&store, NULL);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    ASSERT_EQ_INT(save_saved_game_store(&store, &saved),
                  SAVED_GAME_SAVE_SESSION_ONLY);
    discard_saved_game_store(&store);
    ASSERT_FALSE(store.available);
    ASSERT_NULL(saved_game_store_game(&store));
    ASSERT_FALSE(store.persistent);
}

static void test_discard_saved_game_store_ignores_a_failed_removal(void)
{
    const char* path = TEST_PATH("no_such_directory/discard.txt");
    SavedGameStore store;
    init_saved_game_store(&store, path);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    ASSERT_EQ_INT(save_saved_game_store(&store, &saved),
                  SAVED_GAME_SAVE_SESSION_ONLY);
    discard_saved_game_store(&store);
    ASSERT_FALSE(store.available);
}

static void test_saved_game_store_can_save_again_after_a_discard(void)
{
    const char* path = TEST_PATH("store_after_discard.txt");
    remove_with_temporary(path);
    SavedGameStore store;
    init_saved_game_store(&store, path);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    ASSERT_EQ_INT(save_saved_game_store(&store, &saved),
                  SAVED_GAME_SAVE_STORED);
    discard_saved_game_store(&store);
    ASSERT_EQ_INT(save_saved_game_store(&store, &saved),
                  SAVED_GAME_SAVE_STORED);
    ASSERT_TRUE(store.available);
    bool same = file_holds(path, SAVE_FIXTURE_TEXT);
    remove_with_temporary(path);
    ASSERT_TRUE(same);
}

static void test_saved_game_store_game_is_null_until_a_game_is_kept(void)
{
    SavedGameStore store;
    init_saved_game_store(&store, NULL);
    ASSERT_NULL(saved_game_store_game(&store));
    SavedGame saved;
    build_saved_game_fixture(&saved);
    save_saved_game_store(&store, &saved);
    ASSERT_TRUE(saved_game_store_game(&store) == &store.game);
}

static void test_saved_game_store_never_touches_the_statistics(void)
{
    const char* stats_path = TEST_PATH("store_beside_stats.txt");
    const char* save_path  = TEST_PATH("store_beside_save.txt");
    remove_with_temporary(stats_path);
    remove_with_temporary(save_path);
    StatsStore stats;
    init_stats_store(&stats, stats_path);
    ASSERT_TRUE(record_stats_store_game(&stats, DIFFICULTY_HARD, 90));
    char stats_before[TEST_TEXT_BUFFER_SIZE];
    size_t stats_length =
        read_file(stats_path, stats_before, sizeof stats_before);
    ASSERT_TRUE(stats_length > 0);
    Statistics statistics_before = stats.statistics;

    SavedGameStore store;
    SavedGame saved;
    build_saved_game_fixture(&saved);
    init_saved_game_store(&store, save_path);
    save_saved_game_store(&store, &saved);
    SavedGame loaded;
    load_saved_game_file(save_path, &loaded);
    discard_saved_game_store(&store);
    save_saved_game_store(&store, &saved);
    init_saved_game_store(&store, save_path);

    char stats_after[TEST_TEXT_BUFFER_SIZE];
    size_t after_length =
        read_file(stats_path, stats_after, sizeof stats_after);
    remove_with_temporary(stats_path);
    remove_with_temporary(save_path);
    ASSERT_EQ_INT(after_length, stats_length);
    ASSERT_EQ_INT(memcmp(stats_before, stats_after, stats_length), 0);
    for (int i = 0; i < DIFFICULTY_COUNT; i++)
    {
        ASSERT_EQ_INT(stats.statistics.by_difficulty[i].completed_count,
                      statistics_before.by_difficulty[i].completed_count);
        ASSERT_EQ_INT(stats.statistics.by_difficulty[i].best_seconds,
                      statistics_before.by_difficulty[i].best_seconds);
        ASSERT_EQ_INT(stats.statistics.by_difficulty[i].total_seconds,
                      statistics_before.by_difficulty[i].total_seconds);
    }
}

static void test_saved_game_and_statistics_files_do_not_read_each_other(void)
{
    const char* path = TEST_PATH("store_cross_read.txt");
    StatsStore stats;
    remove_with_temporary(path);
    init_stats_store(&stats, path);
    ASSERT_TRUE(record_stats_store_game(&stats, DIFFICULTY_EASY, 30));
    SavedGame game;
    ASSERT_EQ_INT(load_saved_game_file(path, &game), SAVED_GAME_LOAD_CORRUPTED);
    SavedGame saved;
    build_saved_game_fixture(&saved);
    ASSERT_TRUE(save_saved_game_file(path, &saved));
    Statistics statistics;
    ASSERT_EQ_INT(load_statistics_file(path, &statistics),
                  STATS_LOAD_CORRUPTED);
    remove_with_temporary(path);
}

int main(void)
{
    RUN_TEST(test_saved_game_path_max_shares_the_text_file_limit);
    RUN_TEST(test_save_and_load_saved_game_file_round_trip);
    RUN_TEST(test_save_saved_game_file_writes_the_documented_text);
    RUN_TEST(test_save_saved_game_file_overwrites_an_existing_file);
    RUN_TEST(test_save_saved_game_file_replaces_a_corrupted_file);
    RUN_TEST(test_save_saved_game_file_unwritable_location_fails_cleanly);
    RUN_TEST(test_save_saved_game_file_rejects_too_long_path);
    RUN_TEST(test_load_saved_game_file_missing_file_is_missing);
    RUN_TEST(test_load_saved_game_file_empty_file_is_corrupted);
    RUN_TEST(test_load_saved_game_file_header_only_is_corrupted);
    RUN_TEST(test_load_saved_game_file_garbage_is_corrupted);
    RUN_TEST(test_load_saved_game_file_truncated_file_is_corrupted);
    RUN_TEST(test_load_saved_game_file_oversized_file_is_corrupted);
    RUN_TEST(test_load_saved_game_file_unsupported_version_is_corrupted);
    RUN_TEST(test_load_saved_game_file_accepts_a_version_one_file);
    RUN_TEST(test_save_saved_game_file_round_trips_the_hint_mark);
    RUN_TEST(test_load_saved_game_file_inconsistent_content_is_corrupted);
    RUN_TEST(test_load_saved_game_file_accepts_a_crlf_file);
    RUN_TEST(test_load_saved_game_file_leaves_the_file_alone);
    RUN_TEST(test_init_saved_game_store_without_path_is_memory_only);
    RUN_TEST(test_init_saved_game_store_too_long_path_is_memory_only);
    RUN_TEST(test_init_saved_game_store_missing_file_has_no_game);
    RUN_TEST(test_init_saved_game_store_loads_an_existing_file);
    RUN_TEST(test_init_saved_game_store_corrupted_file_has_no_game);
    RUN_TEST(test_init_saved_game_store_replaces_a_previous_state);
    RUN_TEST(test_save_saved_game_store_without_path_stays_in_memory);
    RUN_TEST(test_save_saved_game_store_keeps_a_copy);
    RUN_TEST(test_save_saved_game_store_persists_for_the_next_run);
    RUN_TEST(test_save_saved_game_store_replaces_the_previous_save);
    RUN_TEST(test_save_saved_game_store_replaces_a_corrupted_file);
    RUN_TEST(test_save_saved_game_store_rejects_an_invalid_game);
    RUN_TEST(test_save_saved_game_store_rejection_keeps_the_previous_save);
    RUN_TEST(test_save_saved_game_store_rejects_an_unsolvable_game);
    RUN_TEST(test_save_saved_game_store_write_failure_keeps_the_game_in_memory);
    RUN_TEST(test_discard_saved_game_store_forgets_the_game_and_the_file);
    RUN_TEST(test_discard_saved_game_store_without_a_game_is_harmless);
    RUN_TEST(test_discard_saved_game_store_memory_only);
    RUN_TEST(test_discard_saved_game_store_ignores_a_failed_removal);
    RUN_TEST(test_saved_game_store_can_save_again_after_a_discard);
    RUN_TEST(test_saved_game_store_game_is_null_until_a_game_is_kept);
    RUN_TEST(test_saved_game_store_never_touches_the_statistics);
    RUN_TEST(test_saved_game_and_statistics_files_do_not_read_each_other);
    return test_harness_result();
}
