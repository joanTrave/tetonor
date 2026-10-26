#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/platform/user_data_dir.h"
#include "../../src/storage/text_file.h"

/* make test recreates this directory before running any test binary. */
#define TEST_TMP_DIR "build/tests/tmp"
#define TEST_PATH(name) (TEST_TMP_DIR "/" name)
#define TEST_BUFFER_SIZE 64
#define TEST_BIG_BUFFER_SIZE 2048
#define TEST_CANARY 'Z'
#define TEST_LENGTH_SENTINEL 777
#define TEST_MAX_LENGTH 16
#define TEST_TEMPORARY_SUFFIX ".tmp"

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

static void remove_with_temporary(const char* path)
{
    char temporary[TETONOR_TEXT_FILE_PATH_MAX + sizeof TEST_TEMPORARY_SUFFIX];
    remove(path);
    snprintf(temporary, sizeof temporary, "%s" TEST_TEMPORARY_SUFFIX, path);
    remove(temporary);
}

static bool temporary_file_exists(const char* path)
{
    char temporary[TETONOR_TEXT_FILE_PATH_MAX + sizeof TEST_TEMPORARY_SUFFIX];
    snprintf(temporary, sizeof temporary, "%s" TEST_TEMPORARY_SUFFIX, path);
    return file_exists(temporary);
}

static void test_text_file_path_max_is_one_kilobyte(void)
{
    ASSERT_EQ_INT(TETONOR_TEXT_FILE_PATH_MAX, 1024);
}

static void test_read_text_file_reads_the_whole_file(void)
{
    const char* path = TEST_PATH("text_file_read.txt");
    remove_with_temporary(path);
    ASSERT_TRUE(write_file(path, "hello", 5));
    char text[TEST_MAX_LENGTH + 1];
    size_t length = TEST_LENGTH_SENTINEL;
    TextFileReadResult result =
        read_text_file(path, text, TEST_MAX_LENGTH, &length);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, TEXT_FILE_READ_OK);
    ASSERT_EQ_INT(length, 5);
    ASSERT_EQ_INT(memcmp(text, "hello", 5), 0);
}

static void test_read_text_file_empty_file_is_ok(void)
{
    const char* path = TEST_PATH("text_file_empty.txt");
    remove_with_temporary(path);
    ASSERT_TRUE(write_file(path, "", 0));
    char text[TEST_MAX_LENGTH + 1];
    size_t length = TEST_LENGTH_SENTINEL;
    TextFileReadResult result =
        read_text_file(path, text, TEST_MAX_LENGTH, &length);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, TEXT_FILE_READ_OK);
    ASSERT_EQ_INT(length, 0);
}

static void test_read_text_file_keeps_binary_content(void)
{
    const char* path   = TEST_PATH("text_file_binary.txt");
    const char bytes[] = {'a', '\0', 'b', '\r', '\n', '\x1a', 'c'};
    remove_with_temporary(path);
    ASSERT_TRUE(write_file(path, bytes, sizeof bytes));
    char text[TEST_MAX_LENGTH + 1];
    size_t length = 0;
    TextFileReadResult result =
        read_text_file(path, text, TEST_MAX_LENGTH, &length);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, TEXT_FILE_READ_OK);
    ASSERT_EQ_INT(length, sizeof bytes);
    ASSERT_EQ_INT(memcmp(text, bytes, sizeof bytes), 0);
}

static void test_read_text_file_exactly_max_length_is_ok(void)
{
    const char* path = TEST_PATH("text_file_exact.txt");
    char bytes[TEST_MAX_LENGTH + 1];
    memset(bytes, 'x', sizeof bytes);
    remove_with_temporary(path);
    ASSERT_TRUE(write_file(path, bytes, TEST_MAX_LENGTH));
    char text[TEST_MAX_LENGTH + 1];
    size_t length = TEST_LENGTH_SENTINEL;
    TextFileReadResult result =
        read_text_file(path, text, TEST_MAX_LENGTH, &length);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, TEXT_FILE_READ_OK);
    ASSERT_EQ_INT(length, TEST_MAX_LENGTH);
}

static void test_read_text_file_longer_than_max_length_fails(void)
{
    const char* path = TEST_PATH("text_file_oversized.txt");
    char bytes[TEST_BIG_BUFFER_SIZE];
    memset(bytes, 'x', sizeof bytes);
    remove_with_temporary(path);
    ASSERT_TRUE(write_file(path, bytes, TEST_MAX_LENGTH + 1));
    char text[TEST_MAX_LENGTH + 1];
    size_t length = TEST_LENGTH_SENTINEL;
    ASSERT_EQ_INT(read_text_file(path, text, TEST_MAX_LENGTH, &length),
                  TEXT_FILE_READ_FAILED);
    ASSERT_EQ_INT(length, TEST_LENGTH_SENTINEL);
    ASSERT_TRUE(write_file(path, bytes, sizeof bytes));
    ASSERT_EQ_INT(read_text_file(path, text, TEST_MAX_LENGTH, &length),
                  TEXT_FILE_READ_FAILED);
    ASSERT_EQ_INT(length, TEST_LENGTH_SENTINEL);
    remove_with_temporary(path);
}

static void test_read_text_file_zero_max_length(void)
{
    const char* path = TEST_PATH("text_file_zero_max.txt");
    remove_with_temporary(path);
    char text[1];
    size_t length = TEST_LENGTH_SENTINEL;
    ASSERT_TRUE(write_file(path, "", 0));
    ASSERT_EQ_INT(read_text_file(path, text, 0, &length), TEXT_FILE_READ_OK);
    ASSERT_EQ_INT(length, 0);
    ASSERT_TRUE(write_file(path, "x", 1));
    length = TEST_LENGTH_SENTINEL;
    ASSERT_EQ_INT(read_text_file(path, text, 0, &length),
                  TEXT_FILE_READ_FAILED);
    ASSERT_EQ_INT(length, TEST_LENGTH_SENTINEL);
    remove_with_temporary(path);
}

static void test_read_text_file_missing_file_is_missing(void)
{
    const char* path = TEST_PATH("text_file_missing.txt");
    remove_with_temporary(path);
    char text[TEST_MAX_LENGTH + 1];
    size_t length = TEST_LENGTH_SENTINEL;
    ASSERT_EQ_INT(read_text_file(path, text, TEST_MAX_LENGTH, &length),
                  TEXT_FILE_READ_MISSING);
    ASSERT_EQ_INT(length, TEST_LENGTH_SENTINEL);
}

static void test_read_text_file_missing_directory_is_missing(void)
{
    char text[TEST_MAX_LENGTH + 1];
    size_t length = TEST_LENGTH_SENTINEL;
    ASSERT_EQ_INT(read_text_file(TEST_PATH("no_such_directory/file.txt"), text,
                                 TEST_MAX_LENGTH, &length),
                  TEXT_FILE_READ_MISSING);
    ASSERT_EQ_INT(length, TEST_LENGTH_SENTINEL);
}

static void test_write_text_file_atomically_creates_the_file(void)
{
    const char* path = TEST_PATH("text_file_write.txt");
    remove_with_temporary(path);
    ASSERT_TRUE(write_text_file_atomically(path, "first\nsecond\n"));
    ASSERT_FALSE(temporary_file_exists(path));
    char text[TEST_BUFFER_SIZE];
    size_t length = read_file(path, text, sizeof text);
    remove_with_temporary(path);
    ASSERT_EQ_INT(length, strlen("first\nsecond\n"));
    ASSERT_EQ_INT(memcmp(text, "first\nsecond\n", length), 0);
}

static void test_write_text_file_atomically_writes_an_empty_text(void)
{
    const char* path = TEST_PATH("text_file_write_empty.txt");
    remove_with_temporary(path);
    ASSERT_TRUE(write_text_file_atomically(path, ""));
    ASSERT_TRUE(file_exists(path));
    char text[TEST_BUFFER_SIZE];
    size_t length = read_file(path, text, sizeof text);
    remove_with_temporary(path);
    ASSERT_EQ_INT(length, 0);
}

static void test_write_text_file_atomically_replaces_a_longer_file(void)
{
    const char* path = TEST_PATH("text_file_replace.txt");
    remove_with_temporary(path);
    ASSERT_TRUE(write_text_file_atomically(path, "a much longer old content"));
    ASSERT_TRUE(write_text_file_atomically(path, "new"));
    char text[TEST_BUFFER_SIZE];
    size_t length = read_file(path, text, sizeof text);
    remove_with_temporary(path);
    ASSERT_EQ_INT(length, 3);
    ASSERT_EQ_INT(memcmp(text, "new", 3), 0);
}

static void test_write_text_file_atomically_replaces_a_binary_file(void)
{
    const char* path = TEST_PATH("text_file_replace_binary.txt");
    char garbage[TEST_BUFFER_SIZE];
    memset(garbage, '\xff', sizeof garbage);
    remove_with_temporary(path);
    ASSERT_TRUE(write_file(path, garbage, sizeof garbage));
    ASSERT_TRUE(write_text_file_atomically(path, "ok\n"));
    char text[TEST_BUFFER_SIZE];
    size_t length = read_file(path, text, sizeof text);
    remove_with_temporary(path);
    ASSERT_EQ_INT(length, 3);
    ASSERT_EQ_INT(memcmp(text, "ok\n", 3), 0);
}

static void test_write_text_file_atomically_keeps_line_ends_as_given(void)
{
    const char* path = TEST_PATH("text_file_line_ends.txt");
    remove_with_temporary(path);
    ASSERT_TRUE(write_text_file_atomically(path, "a\nb\r\nc\n"));
    char text[TEST_BUFFER_SIZE];
    size_t length = read_file(path, text, sizeof text);
    remove_with_temporary(path);
    ASSERT_EQ_INT(length, 7);
    ASSERT_EQ_INT(memcmp(text, "a\nb\r\nc\n", 7), 0);
}

static void test_write_text_file_atomically_missing_directory_fails(void)
{
    const char* path = TEST_PATH("no_such_directory/file.txt");
    ASSERT_FALSE(write_text_file_atomically(path, "text"));
    ASSERT_FALSE(file_exists(path));
    ASSERT_FALSE(temporary_file_exists(path));
}

static void test_write_text_file_atomically_too_long_path_fails(void)
{
    char path[TETONOR_TEXT_FILE_PATH_MAX + 1];
    memset(path, 'a', sizeof path);
    path[TETONOR_TEXT_FILE_PATH_MAX] = '\0';
    ASSERT_FALSE(write_text_file_atomically(path, "text"));
    path[TETONOR_TEXT_FILE_PATH_MAX - 1] = '\0';
    ASSERT_FALSE(write_text_file_atomically(path, "text"));
    path[TETONOR_TEXT_FILE_PATH_MAX - 2] = '\0';
    ASSERT_FALSE(write_text_file_atomically(path, "text"));
}

static void
test_write_text_file_atomically_keeps_the_target_it_cannot_replace(void)
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
    ASSERT_TRUE(write_file(inside, "kept", 4));
    bool written = write_text_file_atomically(directory_file, "text");
    bool kept    = file_exists(inside);
    bool left    = temporary_file_exists(directory_file);
    remove(inside);
    ASSERT_FALSE(written);
    ASSERT_TRUE(kept);
    ASSERT_FALSE(left);
}

int main(void)
{
    RUN_TEST(test_text_file_path_max_is_one_kilobyte);
    RUN_TEST(test_read_text_file_reads_the_whole_file);
    RUN_TEST(test_read_text_file_empty_file_is_ok);
    RUN_TEST(test_read_text_file_keeps_binary_content);
    RUN_TEST(test_read_text_file_exactly_max_length_is_ok);
    RUN_TEST(test_read_text_file_longer_than_max_length_fails);
    RUN_TEST(test_read_text_file_zero_max_length);
    RUN_TEST(test_read_text_file_missing_file_is_missing);
    RUN_TEST(test_read_text_file_missing_directory_is_missing);
    RUN_TEST(test_write_text_file_atomically_creates_the_file);
    RUN_TEST(test_write_text_file_atomically_writes_an_empty_text);
    RUN_TEST(test_write_text_file_atomically_replaces_a_longer_file);
    RUN_TEST(test_write_text_file_atomically_replaces_a_binary_file);
    RUN_TEST(test_write_text_file_atomically_keeps_line_ends_as_given);
    RUN_TEST(test_write_text_file_atomically_missing_directory_fails);
    RUN_TEST(test_write_text_file_atomically_too_long_path_fails);
    RUN_TEST(
        test_write_text_file_atomically_keeps_the_target_it_cannot_replace);
    return test_harness_result();
}
