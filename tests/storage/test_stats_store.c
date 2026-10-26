#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/game/game_timer.h"
#include "../../src/storage/stats_store.h"

/* make test recreates this directory before running any test binary. */
#define TEST_TMP_DIR "build/tests/tmp"
#define TEST_PATH(name) (TEST_TMP_DIR "/" name)
#define TEST_TEXT_BUFFER_SIZE 512
#define TEST_GARBAGE_SIZE 64

#define VALID_HEADER "TETONOR-STATS 1\n"
#define VALID_BODY "easy 0 0 0\nmedium 0 0 0\nhard 0 0 0\n"

static Statistics make_sample_statistics(void)
{
    Statistics sample;
    memset(&sample, 0, sizeof sample);
    sample.by_difficulty[DIFFICULTY_EASY]   = (DifficultyStatistics){3, 12, 61};
    sample.by_difficulty[DIFFICULTY_MEDIUM] = (DifficultyStatistics){1, 90, 90};
    return sample;
}

static Statistics make_other_statistics(void)
{
    Statistics other;
    memset(&other, 0, sizeof other);
    other.by_difficulty[DIFFICULTY_HARD] = (DifficultyStatistics){2, 40, 100};
    return other;
}

static bool statistics_equal(const Statistics* left, const Statistics* right)
{
    for (int i = 0; i < DIFFICULTY_COUNT; i++)
    {
        if (left->by_difficulty[i].completed_count !=
                right->by_difficulty[i].completed_count ||
            left->by_difficulty[i].best_seconds !=
                right->by_difficulty[i].best_seconds ||
            left->by_difficulty[i].total_seconds !=
                right->by_difficulty[i].total_seconds)
        {
            return false;
        }
    }
    return true;
}

static bool statistics_empty(const Statistics* statistics)
{
    Statistics empty;
    memset(&empty, 0, sizeof empty);
    return statistics_equal(statistics, &empty);
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

static void remove_with_temporary(const char* path)
{
    char temporary[TETONOR_STATS_PATH_MAX + 8];
    remove(path);
    snprintf(temporary, sizeof temporary, "%s.tmp", path);
    remove(temporary);
}

static bool temporary_file_exists(const char* path)
{
    char temporary[TETONOR_STATS_PATH_MAX + 8];
    snprintf(temporary, sizeof temporary, "%s.tmp", path);
    return file_exists(temporary);
}

static void test_save_and_load_statistics_round_trip(void)
{
    const char* path = TEST_PATH("round_trip.txt");
    remove_with_temporary(path);
    Statistics sample = make_sample_statistics();
    ASSERT_TRUE(save_statistics_file(path, &sample));
    ASSERT_FALSE(temporary_file_exists(path));
    Statistics loaded = make_other_statistics();
    ASSERT_EQ_INT(load_statistics_file(path, &loaded), STATS_LOAD_LOADED);
    ASSERT_TRUE(statistics_equal(&loaded, &sample));
    remove_with_temporary(path);
}

static void test_save_statistics_writes_the_documented_text(void)
{
    const char* path = TEST_PATH("documented_text.txt");
    remove_with_temporary(path);
    Statistics sample = make_sample_statistics();
    ASSERT_TRUE(save_statistics_file(path, &sample));
    char text[TEST_TEXT_BUFFER_SIZE];
    FILE* file = fopen(path, "rb");
    ASSERT_NOT_NULL(file);
    size_t length = fread(text, 1, sizeof text - 1, file);
    fclose(file);
    text[length] = '\0';
    remove_with_temporary(path);
    ASSERT_TRUE(strcmp(text, "TETONOR-STATS 1\n"
                             "easy 3 12 61\n"
                             "medium 1 90 90\n"
                             "hard 0 0 0\n") == 0);
}

static void test_load_statistics_missing_file_is_empty(void)
{
    const char* path = TEST_PATH("missing.txt");
    remove_with_temporary(path);
    Statistics loaded = make_sample_statistics();
    ASSERT_EQ_INT(load_statistics_file(path, &loaded), STATS_LOAD_MISSING);
    ASSERT_TRUE(statistics_empty(&loaded));
}

static void test_load_statistics_empty_file_is_corrupted(void)
{
    const char* path = TEST_PATH("empty.txt");
    ASSERT_TRUE(write_file(path, "", 0));
    Statistics loaded      = make_sample_statistics();
    StatsLoadResult result = load_statistics_file(path, &loaded);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, STATS_LOAD_CORRUPTED);
    ASSERT_TRUE(statistics_empty(&loaded));
}

static void test_load_statistics_truncated_file_is_corrupted(void)
{
    const char* path  = TEST_PATH("truncated.txt");
    const char text[] = "TETONOR-STATS 1\neasy 3 12 61\nmedium 1 90 90\n"
                        "hard 0 0 0\n";
    ASSERT_TRUE(write_file(path, text, (sizeof text - 1) / 2));
    Statistics loaded      = make_sample_statistics();
    StatsLoadResult result = load_statistics_file(path, &loaded);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, STATS_LOAD_CORRUPTED);
    ASSERT_TRUE(statistics_empty(&loaded));
}

static void test_load_statistics_garbage_file_is_corrupted(void)
{
    const char* path = TEST_PATH("garbage.txt");
    unsigned char garbage[TEST_GARBAGE_SIZE];
    for (size_t i = 0; i < sizeof garbage; i++)
    {
        garbage[i] = (unsigned char)((i * 37u + 11u) % 256u);
    }
    garbage[0] = 0;
    ASSERT_TRUE(write_file(path, garbage, sizeof garbage));
    Statistics loaded      = make_sample_statistics();
    StatsLoadResult result = load_statistics_file(path, &loaded);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, STATS_LOAD_CORRUPTED);
    ASSERT_TRUE(statistics_empty(&loaded));
}

static void test_load_statistics_out_of_range_values_are_corrupted(void)
{
    const char* path       = TEST_PATH("out_of_range.txt");
    const char* contents[] = {
        VALID_HEADER "easy -1 0 0\nmedium 0 0 0\nhard 0 0 0\n",
        VALID_HEADER
        "easy 99999999999999999999 1 1\nmedium 0 0 0\nhard 0 0 0\n",
        VALID_HEADER "easy x y z\nmedium 0 0 0\nhard 0 0 0\n",
        VALID_HEADER "easy 1 400000 400000\nmedium 0 0 0\nhard 0 0 0\n",
        VALID_HEADER "easy 2 10 19\nmedium 0 0 0\nhard 0 0 0\n",
    };
    for (size_t i = 0; i < sizeof contents / sizeof contents[0]; i++)
    {
        ASSERT_TRUE(write_text_file(path, contents[i]));
        Statistics loaded      = make_sample_statistics();
        StatsLoadResult result = load_statistics_file(path, &loaded);
        remove_with_temporary(path);
        ASSERT_EQ_INT(result, STATS_LOAD_CORRUPTED);
        ASSERT_TRUE(statistics_empty(&loaded));
    }
}

static void test_load_statistics_unsupported_version_is_corrupted(void)
{
    const char* path = TEST_PATH("version_two.txt");
    ASSERT_TRUE(write_text_file(path, "TETONOR-STATS 2\n" VALID_BODY));
    Statistics loaded      = make_sample_statistics();
    StatsLoadResult result = load_statistics_file(path, &loaded);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, STATS_LOAD_CORRUPTED);
    ASSERT_TRUE(statistics_empty(&loaded));
}

static void test_load_statistics_oversized_file_is_corrupted(void)
{
    const char* path = TEST_PATH("oversized.txt");
    char text[TETONOR_STATS_TEXT_MAX + 50];
    size_t valid_length = strlen(VALID_HEADER VALID_BODY);
    memset(text, '\n', sizeof text);
    memcpy(text, VALID_HEADER VALID_BODY, valid_length);
    ASSERT_TRUE(write_file(path, text, sizeof text));
    Statistics loaded      = make_sample_statistics();
    StatsLoadResult result = load_statistics_file(path, &loaded);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, STATS_LOAD_CORRUPTED);
    ASSERT_TRUE(statistics_empty(&loaded));
}

static void test_load_statistics_accepts_crlf_file(void)
{
    const char* path = TEST_PATH("crlf.txt");
    ASSERT_TRUE(write_text_file(
        path, "TETONOR-STATS 1\r\neasy 3 12 61\r\nmedium 1 90 90\r\n"
              "hard 0 0 0\r\n"));
    Statistics loaded      = make_other_statistics();
    StatsLoadResult result = load_statistics_file(path, &loaded);
    remove_with_temporary(path);
    Statistics sample = make_sample_statistics();
    ASSERT_EQ_INT(result, STATS_LOAD_LOADED);
    ASSERT_TRUE(statistics_equal(&loaded, &sample));
}

static void test_save_statistics_overwrites_existing_file(void)
{
    const char* path = TEST_PATH("overwrite.txt");
    remove_with_temporary(path);
    Statistics first  = make_sample_statistics();
    Statistics second = make_other_statistics();
    ASSERT_TRUE(save_statistics_file(path, &first));
    ASSERT_TRUE(save_statistics_file(path, &second));
    ASSERT_FALSE(temporary_file_exists(path));
    Statistics loaded;
    StatsLoadResult result = load_statistics_file(path, &loaded);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, STATS_LOAD_LOADED);
    ASSERT_TRUE(statistics_equal(&loaded, &second));
}

static void test_save_statistics_overwrites_corrupted_file(void)
{
    const char* path = TEST_PATH("overwrite_corrupted.txt");
    ASSERT_TRUE(write_text_file(path, "this is not a statistics file"));
    Statistics sample = make_sample_statistics();
    ASSERT_TRUE(save_statistics_file(path, &sample));
    Statistics loaded;
    StatsLoadResult result = load_statistics_file(path, &loaded);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, STATS_LOAD_LOADED);
    ASSERT_TRUE(statistics_equal(&loaded, &sample));
}

static void test_save_statistics_replaces_a_longer_old_file_completely(void)
{
    const char* path = TEST_PATH("replace_longer.txt");
    char filler[TEST_TEXT_BUFFER_SIZE];
    memset(filler, 'x', sizeof filler);
    ASSERT_TRUE(write_file(path, filler, sizeof filler));
    Statistics sample = make_sample_statistics();
    ASSERT_TRUE(save_statistics_file(path, &sample));
    Statistics loaded;
    StatsLoadResult result = load_statistics_file(path, &loaded);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, STATS_LOAD_LOADED);
    ASSERT_TRUE(statistics_equal(&loaded, &sample));
}

static void test_save_statistics_unwritable_location_fails(void)
{
    const char* path  = TEST_PATH("no_such_directory/statistics.txt");
    Statistics sample = make_sample_statistics();
    ASSERT_FALSE(save_statistics_file(path, &sample));
    ASSERT_FALSE(file_exists(path));
    ASSERT_FALSE(temporary_file_exists(path));
}

static void test_save_statistics_rejects_too_long_path(void)
{
    char path[TETONOR_STATS_PATH_MAX + 1];
    memset(path, 'a', sizeof path);
    memcpy(path, TEST_TMP_DIR "/", sizeof(TEST_TMP_DIR "/") - 1);
    path[TETONOR_STATS_PATH_MAX] = '\0';
    Statistics sample            = make_sample_statistics();
    ASSERT_FALSE(save_statistics_file(path, &sample));
    ASSERT_FALSE(file_exists(path));
}

static void test_init_stats_store_without_path_is_memory_only(void)
{
    const char* paths[] = {NULL, ""};
    for (size_t i = 0; i < sizeof paths / sizeof paths[0]; i++)
    {
        StatsStore store;
        memset(&store, 0x5a, sizeof store);
        ASSERT_EQ_INT(init_stats_store(&store, paths[i]), STATS_LOAD_DISABLED);
        ASSERT_FALSE(store.persistent);
        ASSERT_TRUE(statistics_empty(&store.statistics));
        ASSERT_TRUE(record_stats_store_game(&store, DIFFICULTY_EASY, 42));
        ASSERT_EQ_INT(
            store.statistics.by_difficulty[DIFFICULTY_EASY].completed_count, 1);
        ASSERT_EQ_INT(
            store.statistics.by_difficulty[DIFFICULTY_EASY].best_seconds, 42);
    }
}

static void test_init_stats_store_too_long_path_is_memory_only(void)
{
    char path[TETONOR_STATS_PATH_MAX + 1];
    memset(path, 'a', sizeof path);
    memcpy(path, TEST_TMP_DIR "/", sizeof(TEST_TMP_DIR "/") - 1);
    path[TETONOR_STATS_PATH_MAX] = '\0';
    StatsStore store;
    ASSERT_EQ_INT(init_stats_store(&store, path), STATS_LOAD_DISABLED);
    ASSERT_FALSE(store.persistent);
    ASSERT_TRUE(statistics_empty(&store.statistics));
    ASSERT_TRUE(record_stats_store_game(&store, DIFFICULTY_HARD, 5));
    ASSERT_FALSE(file_exists(path));
}

static void test_init_stats_store_missing_file_starts_empty_and_persists(void)
{
    const char* path = TEST_PATH("store_missing.txt");
    remove_with_temporary(path);
    StatsStore store;
    memset(&store, 0x5a, sizeof store);
    ASSERT_EQ_INT(init_stats_store(&store, path), STATS_LOAD_MISSING);
    ASSERT_TRUE(store.persistent);
    ASSERT_TRUE(statistics_empty(&store.statistics));
    ASSERT_FALSE(file_exists(path));
}

static void test_init_stats_store_loads_existing_file(void)
{
    const char* path = TEST_PATH("store_existing.txt");
    remove_with_temporary(path);
    Statistics sample = make_sample_statistics();
    ASSERT_TRUE(save_statistics_file(path, &sample));
    StatsStore store;
    StatsLoadResult result = init_stats_store(&store, path);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, STATS_LOAD_LOADED);
    ASSERT_TRUE(store.persistent);
    ASSERT_TRUE(statistics_equal(&store.statistics, &sample));
    ASSERT_TRUE(strcmp(store.path, path) == 0);
}

static void test_init_stats_store_corrupted_file_starts_empty_and_persists(void)
{
    const char* path = TEST_PATH("store_corrupted.txt");
    ASSERT_TRUE(write_text_file(path, "garbage"));
    StatsStore store;
    StatsLoadResult result = init_stats_store(&store, path);
    ASSERT_EQ_INT(result, STATS_LOAD_CORRUPTED);
    ASSERT_TRUE(store.persistent);
    ASSERT_TRUE(statistics_empty(&store.statistics));
    bool recorded = record_stats_store_game(&store, DIFFICULTY_MEDIUM, 30);
    Statistics loaded;
    StatsLoadResult reloaded = load_statistics_file(path, &loaded);
    remove_with_temporary(path);
    ASSERT_TRUE(recorded);
    ASSERT_EQ_INT(reloaded, STATS_LOAD_LOADED);
    ASSERT_EQ_INT(loaded.by_difficulty[DIFFICULTY_MEDIUM].completed_count, 1);
    ASSERT_EQ_INT(loaded.by_difficulty[DIFFICULTY_MEDIUM].best_seconds, 30);
}

static void test_record_stats_store_game_persists_for_next_session(void)
{
    const char* path = TEST_PATH("store_next_session.txt");
    remove_with_temporary(path);
    StatsStore first;
    init_stats_store(&first, path);
    ASSERT_TRUE(record_stats_store_game(&first, DIFFICULTY_EASY, 50));
    ASSERT_TRUE(record_stats_store_game(&first, DIFFICULTY_EASY, 30));
    StatsStore second;
    StatsLoadResult result = init_stats_store(&second, path);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, STATS_LOAD_LOADED);
    ASSERT_TRUE(statistics_equal(&second.statistics, &first.statistics));
    ASSERT_EQ_INT(
        second.statistics.by_difficulty[DIFFICULTY_EASY].completed_count, 2);
    ASSERT_EQ_INT(second.statistics.by_difficulty[DIFFICULTY_EASY].best_seconds,
                  30);
    ASSERT_EQ_INT(
        second.statistics.by_difficulty[DIFFICULTY_EASY].total_seconds, 80);
}

static void test_record_stats_store_game_save_failure_keeps_memory(void)
{
    const char* path = TEST_PATH("no_such_directory/store.txt");
    StatsStore store;
    init_stats_store(&store, path);
    ASSERT_TRUE(store.persistent);
    ASSERT_FALSE(record_stats_store_game(&store, DIFFICULTY_HARD, 20));
    ASSERT_EQ_INT(
        store.statistics.by_difficulty[DIFFICULTY_HARD].completed_count, 1);
    ASSERT_FALSE(record_stats_store_game(&store, DIFFICULTY_HARD, 10));
    ASSERT_EQ_INT(
        store.statistics.by_difficulty[DIFFICULTY_HARD].completed_count, 2);
    ASSERT_EQ_INT(store.statistics.by_difficulty[DIFFICULTY_HARD].best_seconds,
                  10);
    ASSERT_FALSE(file_exists(path));
}

static void test_record_stats_store_game_rejects_bad_difficulty(void)
{
    const char* path = TEST_PATH("store_bad_difficulty.txt");
    remove_with_temporary(path);
    StatsStore store;
    init_stats_store(&store, path);
    ASSERT_FALSE(record_stats_store_game(&store, -1, 10));
    ASSERT_FALSE(record_stats_store_game(&store, DIFFICULTY_COUNT, 10));
    ASSERT_TRUE(statistics_empty(&store.statistics));
    bool created = file_exists(path);
    remove_with_temporary(path);
    ASSERT_FALSE(created);
}

static void test_record_stats_store_game_saturated_count_saves_nothing(void)
{
    const char* path = TEST_PATH("store_saturated.txt");
    remove_with_temporary(path);
    StatsStore store;
    init_stats_store(&store, path);
    store.statistics.by_difficulty[DIFFICULTY_EASY] = (DifficultyStatistics){
        TETONOR_STATS_MAX_COMPLETED, 1, TETONOR_STATS_MAX_COMPLETED};
    bool recorded = record_stats_store_game(&store, DIFFICULTY_EASY, 5);
    bool created  = file_exists(path);
    remove_with_temporary(path);
    ASSERT_FALSE(recorded);
    ASSERT_FALSE(created);
}

static void test_stats_path_max_is_unchanged_by_the_text_file_extraction(void)
{
    ASSERT_EQ_INT(TETONOR_STATS_PATH_MAX, TETONOR_TEXT_FILE_PATH_MAX);
    ASSERT_EQ_INT(TETONOR_STATS_PATH_MAX, 1024);
}

int main(void)
{
    RUN_TEST(test_save_and_load_statistics_round_trip);
    RUN_TEST(test_save_statistics_writes_the_documented_text);
    RUN_TEST(test_load_statistics_missing_file_is_empty);
    RUN_TEST(test_load_statistics_empty_file_is_corrupted);
    RUN_TEST(test_load_statistics_truncated_file_is_corrupted);
    RUN_TEST(test_load_statistics_garbage_file_is_corrupted);
    RUN_TEST(test_load_statistics_out_of_range_values_are_corrupted);
    RUN_TEST(test_load_statistics_unsupported_version_is_corrupted);
    RUN_TEST(test_load_statistics_oversized_file_is_corrupted);
    RUN_TEST(test_load_statistics_accepts_crlf_file);
    RUN_TEST(test_save_statistics_overwrites_existing_file);
    RUN_TEST(test_save_statistics_overwrites_corrupted_file);
    RUN_TEST(test_save_statistics_replaces_a_longer_old_file_completely);
    RUN_TEST(test_save_statistics_unwritable_location_fails);
    RUN_TEST(test_save_statistics_rejects_too_long_path);
    RUN_TEST(test_init_stats_store_without_path_is_memory_only);
    RUN_TEST(test_init_stats_store_too_long_path_is_memory_only);
    RUN_TEST(test_init_stats_store_missing_file_starts_empty_and_persists);
    RUN_TEST(test_init_stats_store_loads_existing_file);
    RUN_TEST(test_init_stats_store_corrupted_file_starts_empty_and_persists);
    RUN_TEST(test_record_stats_store_game_persists_for_next_session);
    RUN_TEST(test_record_stats_store_game_save_failure_keeps_memory);
    RUN_TEST(test_record_stats_store_game_rejects_bad_difficulty);
    RUN_TEST(test_record_stats_store_game_saturated_count_saves_nothing);
    RUN_TEST(test_stats_path_max_is_unchanged_by_the_text_file_extraction);
    return test_harness_result();
}
