#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/platform/user_data_dir.h"
#include "../../src/storage/stats_store.h"
#include "../../src/storage/text_file.h"

/* make test recreates this directory before running any test binary. */
#define TEST_TMP_DIR "build/tests/tmp"
#define TEST_PATH(name) (TEST_TMP_DIR "/" name)
#define TEST_CAPACITY 16
#define TEST_BUFFER_SIZE 64
#define TEST_CANARY 'Z'

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

static bool write_bytes(const char* path, const void* bytes, size_t length)
{
    FILE* file = fopen(path, "wb");
    if (file == NULL)
    {
        return false;
    }
    size_t written = fwrite(bytes, 1, length, file);
    return fclose(file) == 0 && written == length;
}

static void remove_with_temporary(const char* path)
{
    char temporary[TETONOR_TEXT_FILE_PATH_MAX + 8];
    remove(path);
    snprintf(temporary, sizeof temporary, "%s.tmp", path);
    remove(temporary);
}

static bool temporary_file_exists(const char* path)
{
    char temporary[TETONOR_TEXT_FILE_PATH_MAX + 8];
    snprintf(temporary, sizeof temporary, "%s.tmp", path);
    return file_exists(temporary);
}

static void test_text_file_path_limit_is_shared_with_the_stats_store(void)
{
    ASSERT_EQ_INT(TETONOR_TEXT_FILE_PATH_MAX, 1024);
    ASSERT_EQ_INT(TETONOR_STATS_PATH_MAX, TETONOR_TEXT_FILE_PATH_MAX);
}

static void test_write_then_read_text_file_round_trips(void)
{
    const char* path = TEST_PATH("text_round_trip.txt");
    remove_with_temporary(path);
    ASSERT_TRUE(write_text_file_atomic(path, "hello\nworld\n"));
    ASSERT_FALSE(temporary_file_exists(path));
    char text[TEST_BUFFER_SIZE];
    size_t length = 0;
    TextFileReadResult result =
        read_text_file(path, text, sizeof text, &length);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, TEXT_FILE_READ_OK);
    ASSERT_EQ_INT(length, strlen("hello\nworld\n"));
    ASSERT_TRUE(memcmp(text, "hello\nworld\n", length) == 0);
}

static void test_read_text_file_missing_file_is_missing(void)
{
    const char* path = TEST_PATH("text_missing.txt");
    remove_with_temporary(path);
    char text[TEST_BUFFER_SIZE];
    size_t length = 0;
    ASSERT_EQ_INT(read_text_file(path, text, sizeof text, &length),
                  TEXT_FILE_READ_MISSING);
}

static void test_read_text_file_empty_file_is_ok_with_length_zero(void)
{
    const char* path = TEST_PATH("text_empty.txt");
    ASSERT_TRUE(write_bytes(path, "", 0));
    char text[TEST_BUFFER_SIZE];
    size_t length = 99;
    TextFileReadResult result =
        read_text_file(path, text, sizeof text, &length);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, TEXT_FILE_READ_OK);
    ASSERT_EQ_INT(length, 0);
}

static void test_read_text_file_one_byte_below_capacity_is_ok(void)
{
    const char* path = TEST_PATH("text_below_capacity.txt");
    char bytes[TEST_CAPACITY];
    memset(bytes, 'a', sizeof bytes);
    ASSERT_TRUE(write_bytes(path, bytes, TEST_CAPACITY - 1));
    char text[TEST_CAPACITY];
    size_t length = 0;
    TextFileReadResult result =
        read_text_file(path, text, TEST_CAPACITY, &length);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, TEXT_FILE_READ_OK);
    ASSERT_EQ_INT(length, TEST_CAPACITY - 1);
    ASSERT_TRUE(memcmp(text, bytes, length) == 0);
}

static void test_read_text_file_capacity_bytes_or_more_fails(void)
{
    const char* path = TEST_PATH("text_at_capacity.txt");
    char bytes[TEST_CAPACITY + 8];
    memset(bytes, 'a', sizeof bytes);
    char text[TEST_CAPACITY];
    size_t length = 0;
    ASSERT_TRUE(write_bytes(path, bytes, TEST_CAPACITY));
    ASSERT_EQ_INT(read_text_file(path, text, TEST_CAPACITY, &length),
                  TEXT_FILE_READ_FAILED);
    ASSERT_TRUE(write_bytes(path, bytes, TEST_CAPACITY + 1));
    ASSERT_EQ_INT(read_text_file(path, text, TEST_CAPACITY, &length),
                  TEXT_FILE_READ_FAILED);
    ASSERT_TRUE(write_bytes(path, bytes, sizeof bytes));
    ASSERT_EQ_INT(read_text_file(path, text, TEST_CAPACITY, &length),
                  TEXT_FILE_READ_FAILED);
    remove_with_temporary(path);
}

static void test_read_text_file_never_writes_past_capacity(void)
{
    const char* path = TEST_PATH("text_no_overrun.txt");
    char bytes[TEST_CAPACITY * 2];
    memset(bytes, 'a', sizeof bytes);
    ASSERT_TRUE(write_bytes(path, bytes, sizeof bytes));
    char guarded[TEST_CAPACITY * 2];
    memset(guarded, TEST_CANARY, sizeof guarded);
    size_t length = 0;
    read_text_file(path, guarded, TEST_CAPACITY, &length);
    remove_with_temporary(path);
    for (size_t i = TEST_CAPACITY; i < sizeof guarded; i++)
    {
        ASSERT_EQ_INT(guarded[i], TEST_CANARY);
    }
}

static void test_write_text_file_atomic_overwrites_an_existing_file(void)
{
    const char* path = TEST_PATH("text_overwrite.txt");
    remove_with_temporary(path);
    ASSERT_TRUE(write_text_file_atomic(path, "first version"));
    ASSERT_TRUE(write_text_file_atomic(path, "second"));
    ASSERT_FALSE(temporary_file_exists(path));
    char text[TEST_BUFFER_SIZE];
    size_t length = 0;
    TextFileReadResult result =
        read_text_file(path, text, sizeof text, &length);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, TEXT_FILE_READ_OK);
    ASSERT_EQ_INT(length, strlen("second"));
    ASSERT_TRUE(memcmp(text, "second", length) == 0);
}

static void test_write_text_file_atomic_replaces_a_longer_old_file(void)
{
    const char* path = TEST_PATH("text_replace_longer.txt");
    char filler[TEST_BUFFER_SIZE];
    memset(filler, 'x', sizeof filler);
    ASSERT_TRUE(write_bytes(path, filler, sizeof filler));
    ASSERT_TRUE(write_text_file_atomic(path, "short"));
    char text[TEST_BUFFER_SIZE];
    size_t length = 0;
    TextFileReadResult result =
        read_text_file(path, text, sizeof text, &length);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, TEXT_FILE_READ_OK);
    ASSERT_EQ_INT(length, strlen("short"));
}

static void test_write_text_file_atomic_empty_text_writes_an_empty_file(void)
{
    const char* path = TEST_PATH("text_empty_write.txt");
    remove_with_temporary(path);
    ASSERT_TRUE(write_text_file_atomic(path, ""));
    char text[TEST_BUFFER_SIZE];
    size_t length = 99;
    TextFileReadResult result =
        read_text_file(path, text, sizeof text, &length);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, TEXT_FILE_READ_OK);
    ASSERT_EQ_INT(length, 0);
}

static void test_write_text_file_atomic_unwritable_location_fails(void)
{
    const char* path = TEST_PATH("no_such_directory/text.txt");
    ASSERT_FALSE(write_text_file_atomic(path, "text"));
    ASSERT_FALSE(file_exists(path));
    ASSERT_FALSE(temporary_file_exists(path));
}

static void test_write_text_file_atomic_rejects_too_long_path(void)
{
    char path[TETONOR_TEXT_FILE_PATH_MAX + 1];
    memset(path, 'a', sizeof path);
    memcpy(path, TEST_TMP_DIR "/", sizeof(TEST_TMP_DIR "/") - 1);
    path[TETONOR_TEXT_FILE_PATH_MAX] = '\0';
    ASSERT_FALSE(write_text_file_atomic(path, "text"));
    ASSERT_FALSE(file_exists(path));
    path[TETONOR_TEXT_FILE_PATH_MAX - 1] = '\0';
    path[TETONOR_TEXT_FILE_PATH_MAX - 2] = 'b';
    ASSERT_FALSE(file_exists(path));
}

static void test_read_text_file_keeps_binary_content(void)
{
    const char* path   = TEST_PATH("text_binary.txt");
    const char bytes[] = {'a', '\0', 'b', '\r', '\n', '\x1a', 'c'};
    ASSERT_TRUE(write_bytes(path, bytes, sizeof bytes));
    char text[TEST_CAPACITY];
    size_t length = 0;
    TextFileReadResult result =
        read_text_file(path, text, TEST_CAPACITY, &length);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, TEXT_FILE_READ_OK);
    ASSERT_EQ_INT(length, sizeof bytes);
    ASSERT_TRUE(memcmp(text, bytes, sizeof bytes) == 0);
}

static void test_read_text_file_missing_directory_is_missing(void)
{
    char text[TEST_BUFFER_SIZE];
    size_t length = 0;
    ASSERT_EQ_INT(read_text_file(TEST_PATH("no_such_directory/text.txt"), text,
                                 sizeof text, &length),
                  TEXT_FILE_READ_MISSING);
}

static void test_write_text_file_atomic_keeps_line_ends_as_given(void)
{
    const char* path = TEST_PATH("text_line_ends.txt");
    remove_with_temporary(path);
    ASSERT_TRUE(write_text_file_atomic(path, "a\nb\r\nc\n"));
    char text[TEST_BUFFER_SIZE];
    size_t length = 0;
    TextFileReadResult result =
        read_text_file(path, text, sizeof text, &length);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, TEXT_FILE_READ_OK);
    ASSERT_EQ_INT(length, 7);
    ASSERT_TRUE(memcmp(text, "a\nb\r\nc\n", 7) == 0);
}

/* The target is a non-empty directory: the write fails, the directory and its
 * content survive and no temporary file is left behind. */
static void test_write_text_file_atomic_keeps_the_target_it_cannot_replace(void)
{
    char directory_file[TETONOR_TEXT_FILE_PATH_MAX];
    ASSERT_TRUE(prepare_stats_file_path(current_user_data_platform(),
                                        TEST_TMP_DIR, directory_file,
                                        sizeof directory_file));
    char* last_separator = strrchr(directory_file, '/');
    char* last_backslash = strrchr(directory_file, '\\');
    if (last_backslash != NULL &&
        (last_separator == NULL || last_backslash > last_separator))
    {
        last_separator = last_backslash;
    }
    ASSERT_NOT_NULL(last_separator);
    char inside[TETONOR_TEXT_FILE_PATH_MAX];
    *last_separator = '\0';
    snprintf(inside, sizeof inside, "%s/keep.txt", directory_file);
    ASSERT_TRUE(write_bytes(inside, "kept", 4));
    bool written = write_text_file_atomic(directory_file, "text");
    bool kept    = file_exists(inside);
    bool left    = temporary_file_exists(directory_file);
    remove(inside);
    ASSERT_FALSE(written);
    ASSERT_TRUE(kept);
    ASSERT_FALSE(left);
}

int main(void)
{
    RUN_TEST(test_text_file_path_limit_is_shared_with_the_stats_store);
    RUN_TEST(test_write_then_read_text_file_round_trips);
    RUN_TEST(test_read_text_file_missing_file_is_missing);
    RUN_TEST(test_read_text_file_empty_file_is_ok_with_length_zero);
    RUN_TEST(test_read_text_file_one_byte_below_capacity_is_ok);
    RUN_TEST(test_read_text_file_capacity_bytes_or_more_fails);
    RUN_TEST(test_read_text_file_never_writes_past_capacity);
    RUN_TEST(test_write_text_file_atomic_overwrites_an_existing_file);
    RUN_TEST(test_write_text_file_atomic_replaces_a_longer_old_file);
    RUN_TEST(test_write_text_file_atomic_empty_text_writes_an_empty_file);
    RUN_TEST(test_write_text_file_atomic_unwritable_location_fails);
    RUN_TEST(test_write_text_file_atomic_rejects_too_long_path);
    RUN_TEST(test_read_text_file_keeps_binary_content);
    RUN_TEST(test_read_text_file_missing_directory_is_missing);
    RUN_TEST(test_write_text_file_atomic_keeps_line_ends_as_given);
    RUN_TEST(test_write_text_file_atomic_keeps_the_target_it_cannot_replace);
    return test_harness_result();
}
