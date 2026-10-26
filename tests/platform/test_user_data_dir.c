#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/platform/user_data_dir.h"

/* make test recreates this directory before running any test binary. */
#define TEST_TMP_DIR "build/tests/tmp"
#define TEST_PATH_BUFFER_SIZE 512
#define TEST_CANARY 'Z'

#define LINUX_PATH "/home/ana/.local/share/tetonor/statistics.txt"
#define MACOS_PATH                                                             \
    "/Users/ana/Library/Application Support/Tetonor/statistics.txt"
#define WINDOWS_PATH "C:\\Users\\ana\\AppData\\Local\\Tetonor\\statistics.txt"

static void test_user_data_env_names(void)
{
    ASSERT_TRUE(strcmp(user_data_env_name(USER_DATA_PLATFORM_WINDOWS),
                       "LOCALAPPDATA") == 0);
    ASSERT_TRUE(strcmp(user_data_env_name(USER_DATA_PLATFORM_LINUX), "HOME") ==
                0);
    ASSERT_TRUE(strcmp(user_data_env_name(USER_DATA_PLATFORM_MACOS), "HOME") ==
                0);
}

static void test_current_user_data_platform_is_known(void)
{
    UserDataPlatform platform = current_user_data_platform();
    ASSERT_TRUE(platform == USER_DATA_PLATFORM_LINUX ||
                platform == USER_DATA_PLATFORM_MACOS ||
                platform == USER_DATA_PLATFORM_WINDOWS);
    const char* name = user_data_env_name(platform);
    ASSERT_NOT_NULL(name);
    ASSERT_TRUE(strlen(name) > 0);
}

static void test_build_stats_file_path_linux(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_TRUE(build_stats_file_path(USER_DATA_PLATFORM_LINUX, "/home/ana",
                                      path, sizeof path));
    ASSERT_TRUE(strcmp(path, LINUX_PATH) == 0);
}

static void test_build_stats_file_path_macos(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_TRUE(build_stats_file_path(USER_DATA_PLATFORM_MACOS, "/Users/ana",
                                      path, sizeof path));
    ASSERT_TRUE(strcmp(path, MACOS_PATH) == 0);
}

static void test_build_stats_file_path_windows(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_TRUE(build_stats_file_path(USER_DATA_PLATFORM_WINDOWS,
                                      "C:\\Users\\ana\\AppData\\Local", path,
                                      sizeof path));
    ASSERT_TRUE(strcmp(path, WINDOWS_PATH) == 0);
}

static void test_build_stats_file_path_base_with_trailing_separator(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_TRUE(build_stats_file_path(USER_DATA_PLATFORM_LINUX, "/home/ana/",
                                      path, sizeof path));
    ASSERT_TRUE(strcmp(path, LINUX_PATH) == 0);
    ASSERT_TRUE(build_stats_file_path(USER_DATA_PLATFORM_MACOS, "/Users/ana/",
                                      path, sizeof path));
    ASSERT_TRUE(strcmp(path, MACOS_PATH) == 0);
    ASSERT_TRUE(build_stats_file_path(USER_DATA_PLATFORM_WINDOWS, "C:\\x\\",
                                      path, sizeof path));
    ASSERT_TRUE(strcmp(path, "C:\\x\\Tetonor\\statistics.txt") == 0);
}

static void test_build_stats_file_path_windows_base_with_forward_slash(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_TRUE(build_stats_file_path(USER_DATA_PLATFORM_WINDOWS, "C:/x/", path,
                                      sizeof path));
    ASSERT_TRUE(strcmp(path, "C:/x/Tetonor\\statistics.txt") == 0);
}

static void test_build_stats_file_path_rejects_missing_base(void)
{
    const UserDataPlatform platforms[] = {USER_DATA_PLATFORM_LINUX,
                                          USER_DATA_PLATFORM_MACOS,
                                          USER_DATA_PLATFORM_WINDOWS};
    for (size_t i = 0; i < sizeof platforms / sizeof platforms[0]; i++)
    {
        char path[TEST_PATH_BUFFER_SIZE];
        memset(path, TEST_CANARY, sizeof path);
        ASSERT_FALSE(
            build_stats_file_path(platforms[i], NULL, path, sizeof path));
        ASSERT_EQ_INT(path[0], '\0');
        memset(path, TEST_CANARY, sizeof path);
        ASSERT_FALSE(
            build_stats_file_path(platforms[i], "", path, sizeof path));
        ASSERT_EQ_INT(path[0], '\0');
    }
}

static void test_build_stats_file_path_rejects_small_buffer(void)
{
    const struct
    {
        UserDataPlatform platform;
        const char* base;
        const char* expected;
    } cases[] = {
        {USER_DATA_PLATFORM_LINUX, "/home/ana", LINUX_PATH},
        {USER_DATA_PLATFORM_MACOS, "/Users/ana", MACOS_PATH},
        {USER_DATA_PLATFORM_WINDOWS, "C:\\Users\\ana\\AppData\\Local",
         WINDOWS_PATH},
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++)
    {
        char path[TEST_PATH_BUFFER_SIZE];
        size_t length = strlen(cases[i].expected);
        memset(path, TEST_CANARY, sizeof path);
        ASSERT_FALSE(build_stats_file_path(cases[i].platform, cases[i].base,
                                           path, length));
        ASSERT_EQ_INT(path[0], '\0');
        ASSERT_TRUE(build_stats_file_path(cases[i].platform, cases[i].base,
                                          path, length + 1));
        ASSERT_TRUE(strcmp(path, cases[i].expected) == 0);
    }
}

static void test_build_stats_file_path_rejects_empty_buffer(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    memset(path, TEST_CANARY, sizeof path);
    ASSERT_FALSE(
        build_stats_file_path(USER_DATA_PLATFORM_LINUX, "/home/ana", path, 0));
    ASSERT_EQ_INT(path[0], TEST_CANARY);
}

static void test_prepare_stats_file_path_creates_directories(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_TRUE(prepare_stats_file_path(current_user_data_platform(),
                                        TEST_TMP_DIR, path, sizeof path));
    FILE* file = fopen(path, "wb");
    ASSERT_NOT_NULL(file);
    fclose(file);
    remove(path);
}

static void test_prepare_stats_file_path_matches_build_stats_file_path(void)
{
    char prepared[TEST_PATH_BUFFER_SIZE];
    char built[TEST_PATH_BUFFER_SIZE];
    UserDataPlatform platform = current_user_data_platform();
    ASSERT_TRUE(prepare_stats_file_path(platform, TEST_TMP_DIR, prepared,
                                        sizeof prepared));
    ASSERT_TRUE(
        build_stats_file_path(platform, TEST_TMP_DIR, built, sizeof built));
    ASSERT_TRUE(strcmp(prepared, built) == 0);
}

static void test_prepare_stats_file_path_is_idempotent(void)
{
    char first[TEST_PATH_BUFFER_SIZE];
    char second[TEST_PATH_BUFFER_SIZE];
    UserDataPlatform platform = current_user_data_platform();
    ASSERT_TRUE(
        prepare_stats_file_path(platform, TEST_TMP_DIR, first, sizeof first));
    ASSERT_TRUE(
        prepare_stats_file_path(platform, TEST_TMP_DIR, second, sizeof second));
    ASSERT_TRUE(strcmp(first, second) == 0);
}

static void test_prepare_stats_file_path_keeps_existing_files(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    UserDataPlatform platform = current_user_data_platform();
    ASSERT_TRUE(
        prepare_stats_file_path(platform, TEST_TMP_DIR, path, sizeof path));
    FILE* file = fopen(path, "wb");
    ASSERT_NOT_NULL(file);
    fputs("kept", file);
    fclose(file);
    char again[TEST_PATH_BUFFER_SIZE];
    bool prepared =
        prepare_stats_file_path(platform, TEST_TMP_DIR, again, sizeof again);
    char content[8] = {0};
    file            = fopen(path, "rb");
    ASSERT_NOT_NULL(file);
    size_t length = fread(content, 1, sizeof content - 1, file);
    fclose(file);
    remove(path);
    ASSERT_TRUE(prepared);
    ASSERT_EQ_INT(length, 4);
    ASSERT_TRUE(strcmp(content, "kept") == 0);
}

static void test_prepare_stats_file_path_fails_under_a_file(void)
{
    const char* base = TEST_TMP_DIR "/platform_base_is_a_file.txt";
    FILE* file       = fopen(base, "wb");
    ASSERT_NOT_NULL(file);
    fclose(file);
    char path[TEST_PATH_BUFFER_SIZE];
    bool prepared = prepare_stats_file_path(current_user_data_platform(), base,
                                            path, sizeof path);
    remove(base);
    ASSERT_FALSE(prepared);
}

static void test_prepare_stats_file_path_does_not_create_the_base(void)
{
    const char* base = TEST_TMP_DIR "/platform_missing_base";
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_FALSE(prepare_stats_file_path(current_user_data_platform(), base,
                                         path, sizeof path));
    FILE* probe = fopen(TEST_TMP_DIR "/platform_missing_base/probe", "wb");
    if (probe != NULL)
    {
        fclose(probe);
        remove(TEST_TMP_DIR "/platform_missing_base/probe");
    }
    ASSERT_NULL(probe);
}

static void test_prepare_stats_file_path_rejects_missing_base(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    memset(path, TEST_CANARY, sizeof path);
    ASSERT_FALSE(prepare_stats_file_path(current_user_data_platform(), NULL,
                                         path, sizeof path));
    ASSERT_FALSE(prepare_stats_file_path(current_user_data_platform(), "", path,
                                         sizeof path));
}

static void test_prepare_stats_file_path_rejects_small_buffer(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_FALSE(prepare_stats_file_path(current_user_data_platform(),
                                         TEST_TMP_DIR, path, 8));
}

#define LINUX_CONFIG_PATH "/home/ana/.local/share/tetonor/config.txt"
#define MACOS_CONFIG_PATH                                                      \
    "/Users/ana/Library/Application Support/Tetonor/config.txt"
#define WINDOWS_CONFIG_PATH                                                    \
    "C:\\Users\\ana\\AppData\\Local\\Tetonor\\config.txt"
#define TEST_CONFIG_FILE_NAME "config.txt"
#define TEST_STATS_FILE_NAME "statistics.txt"

static const struct
{
    UserDataPlatform platform;
    const char* base;
    const char* stats_path;
    const char* config_path;
} platform_cases[] = {
    {USER_DATA_PLATFORM_LINUX, "/home/ana", LINUX_PATH, LINUX_CONFIG_PATH},
    {USER_DATA_PLATFORM_MACOS, "/Users/ana", MACOS_PATH, MACOS_CONFIG_PATH},
    {USER_DATA_PLATFORM_WINDOWS, "C:\\Users\\ana\\AppData\\Local", WINDOWS_PATH,
     WINDOWS_CONFIG_PATH},
};

static size_t directory_length(const char* path)
{
    size_t last = 0;
    for (size_t i = 0; path[i] != '\0'; i++)
    {
        if (path[i] == '/' || path[i] == '\\')
        {
            last = i + 1;
        }
    }
    return last;
}

static void test_build_user_data_file_path_config_on_every_platform(void)
{
    for (size_t i = 0; i < sizeof platform_cases / sizeof platform_cases[0];
         i++)
    {
        char path[TEST_PATH_BUFFER_SIZE];
        ASSERT_TRUE(build_user_data_file_path(
            platform_cases[i].platform, platform_cases[i].base,
            TEST_CONFIG_FILE_NAME, path, sizeof path));
        ASSERT_TRUE(strcmp(path, platform_cases[i].config_path) == 0);
    }
}

static void
test_build_user_data_file_path_config_shares_the_stats_directory(void)
{
    for (size_t i = 0; i < sizeof platform_cases / sizeof platform_cases[0];
         i++)
    {
        char config[TEST_PATH_BUFFER_SIZE];
        char stats[TEST_PATH_BUFFER_SIZE];
        ASSERT_TRUE(build_user_data_file_path(
            platform_cases[i].platform, platform_cases[i].base,
            TEST_CONFIG_FILE_NAME, config, sizeof config));
        ASSERT_TRUE(build_stats_file_path(platform_cases[i].platform,
                                          platform_cases[i].base, stats,
                                          sizeof stats));
        size_t directory = directory_length(stats);
        ASSERT_EQ_INT(directory_length(config), directory);
        ASSERT_TRUE(strncmp(config, stats, directory) == 0);
        ASSERT_TRUE(strcmp(config + directory, TEST_CONFIG_FILE_NAME) == 0);
    }
}

static void test_build_stats_file_path_equals_the_generic_function(void)
{
    for (size_t i = 0; i < sizeof platform_cases / sizeof platform_cases[0];
         i++)
    {
        char generic[TEST_PATH_BUFFER_SIZE];
        char wrapper[TEST_PATH_BUFFER_SIZE];
        ASSERT_TRUE(build_user_data_file_path(
            platform_cases[i].platform, platform_cases[i].base,
            TEST_STATS_FILE_NAME, generic, sizeof generic));
        ASSERT_TRUE(build_stats_file_path(platform_cases[i].platform,
                                          platform_cases[i].base, wrapper,
                                          sizeof wrapper));
        ASSERT_TRUE(strcmp(generic, wrapper) == 0);
        ASSERT_TRUE(strcmp(wrapper, platform_cases[i].stats_path) == 0);
    }
}

static void test_build_user_data_file_path_base_with_trailing_separator(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_TRUE(build_user_data_file_path(USER_DATA_PLATFORM_LINUX,
                                          "/home/ana/", TEST_CONFIG_FILE_NAME,
                                          path, sizeof path));
    ASSERT_TRUE(strcmp(path, LINUX_CONFIG_PATH) == 0);
    ASSERT_TRUE(build_user_data_file_path(USER_DATA_PLATFORM_WINDOWS, "C:\\x\\",
                                          TEST_CONFIG_FILE_NAME, path,
                                          sizeof path));
    ASSERT_TRUE(strcmp(path, "C:\\x\\Tetonor\\config.txt") == 0);
}

static void test_build_user_data_file_path_rejects_missing_file_name(void)
{
    for (size_t i = 0; i < sizeof platform_cases / sizeof platform_cases[0];
         i++)
    {
        char path[TEST_PATH_BUFFER_SIZE];
        memset(path, TEST_CANARY, sizeof path);
        ASSERT_FALSE(build_user_data_file_path(platform_cases[i].platform,
                                               platform_cases[i].base, NULL,
                                               path, sizeof path));
        ASSERT_EQ_INT(path[0], '\0');
        memset(path, TEST_CANARY, sizeof path);
        ASSERT_FALSE(build_user_data_file_path(platform_cases[i].platform,
                                               platform_cases[i].base, "", path,
                                               sizeof path));
        ASSERT_EQ_INT(path[0], '\0');
    }
}

static void test_build_user_data_file_path_rejects_missing_base(void)
{
    for (size_t i = 0; i < sizeof platform_cases / sizeof platform_cases[0];
         i++)
    {
        char path[TEST_PATH_BUFFER_SIZE];
        memset(path, TEST_CANARY, sizeof path);
        ASSERT_FALSE(build_user_data_file_path(platform_cases[i].platform, NULL,
                                               TEST_CONFIG_FILE_NAME, path,
                                               sizeof path));
        ASSERT_EQ_INT(path[0], '\0');
        memset(path, TEST_CANARY, sizeof path);
        ASSERT_FALSE(build_user_data_file_path(platform_cases[i].platform, "",
                                               TEST_CONFIG_FILE_NAME, path,
                                               sizeof path));
        ASSERT_EQ_INT(path[0], '\0');
    }
}

static void test_build_user_data_file_path_rejects_small_buffer(void)
{
    for (size_t i = 0; i < sizeof platform_cases / sizeof platform_cases[0];
         i++)
    {
        char path[TEST_PATH_BUFFER_SIZE];
        size_t length = strlen(platform_cases[i].config_path);
        memset(path, TEST_CANARY, sizeof path);
        ASSERT_FALSE(build_user_data_file_path(
            platform_cases[i].platform, platform_cases[i].base,
            TEST_CONFIG_FILE_NAME, path, length));
        ASSERT_EQ_INT(path[0], '\0');
        ASSERT_TRUE(build_user_data_file_path(
            platform_cases[i].platform, platform_cases[i].base,
            TEST_CONFIG_FILE_NAME, path, length + 1));
        ASSERT_TRUE(strcmp(path, platform_cases[i].config_path) == 0);
    }
}

static void test_build_user_data_file_path_rejects_empty_buffer(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    memset(path, TEST_CANARY, sizeof path);
    ASSERT_FALSE(build_user_data_file_path(
        USER_DATA_PLATFORM_LINUX, "/home/ana", TEST_CONFIG_FILE_NAME, path, 0));
    ASSERT_EQ_INT(path[0], TEST_CANARY);
}

static void test_prepare_user_data_file_path_creates_the_directory(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_TRUE(prepare_user_data_file_path(current_user_data_platform(),
                                            TEST_TMP_DIR, TEST_CONFIG_FILE_NAME,
                                            path, sizeof path));
    FILE* file = fopen(path, "wb");
    ASSERT_NOT_NULL(file);
    fclose(file);
    remove(path);
}

static void test_prepare_user_data_file_path_matches_the_build_function(void)
{
    char prepared[TEST_PATH_BUFFER_SIZE];
    char built[TEST_PATH_BUFFER_SIZE];
    UserDataPlatform platform = current_user_data_platform();
    ASSERT_TRUE(prepare_user_data_file_path(platform, TEST_TMP_DIR,
                                            TEST_CONFIG_FILE_NAME, prepared,
                                            sizeof prepared));
    ASSERT_TRUE(build_user_data_file_path(
        platform, TEST_TMP_DIR, TEST_CONFIG_FILE_NAME, built, sizeof built));
    ASSERT_TRUE(strcmp(prepared, built) == 0);
}

static void test_prepare_user_data_file_path_shares_the_stats_directory(void)
{
    char config[TEST_PATH_BUFFER_SIZE];
    char stats[TEST_PATH_BUFFER_SIZE];
    UserDataPlatform platform = current_user_data_platform();
    ASSERT_TRUE(prepare_user_data_file_path(
        platform, TEST_TMP_DIR, TEST_CONFIG_FILE_NAME, config, sizeof config));
    ASSERT_TRUE(
        prepare_stats_file_path(platform, TEST_TMP_DIR, stats, sizeof stats));
    size_t directory = directory_length(stats);
    ASSERT_EQ_INT(directory_length(config), directory);
    ASSERT_TRUE(strncmp(config, stats, directory) == 0);
}

static void test_prepare_user_data_file_path_rejects_bad_arguments(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    UserDataPlatform platform = current_user_data_platform();
    ASSERT_FALSE(prepare_user_data_file_path(
        platform, NULL, TEST_CONFIG_FILE_NAME, path, sizeof path));
    ASSERT_FALSE(prepare_user_data_file_path(
        platform, "", TEST_CONFIG_FILE_NAME, path, sizeof path));
    ASSERT_FALSE(prepare_user_data_file_path(platform, TEST_TMP_DIR, NULL, path,
                                             sizeof path));
    ASSERT_FALSE(prepare_user_data_file_path(platform, TEST_TMP_DIR, "", path,
                                             sizeof path));
    ASSERT_FALSE(prepare_user_data_file_path(platform, TEST_TMP_DIR,
                                             TEST_CONFIG_FILE_NAME, path, 8));
}

static void test_prepare_user_data_file_path_fails_under_a_file(void)
{
    const char* base = TEST_TMP_DIR "/platform_config_base_is_a_file.txt";
    FILE* file       = fopen(base, "wb");
    ASSERT_NOT_NULL(file);
    fclose(file);
    char path[TEST_PATH_BUFFER_SIZE];
    bool prepared =
        prepare_user_data_file_path(current_user_data_platform(), base,
                                    TEST_CONFIG_FILE_NAME, path, sizeof path);
    remove(base);
    ASSERT_FALSE(prepared);
}

#define LINUX_SAVED_PATH "/home/ana/.local/share/tetonor/saved_game.txt"
#define MACOS_SAVED_PATH                                                       \
    "/Users/ana/Library/Application Support/Tetonor/saved_game.txt"
#define WINDOWS_SAVED_PATH                                                     \
    "C:\\Users\\ana\\AppData\\Local\\Tetonor\\saved_game.txt"

static void test_build_saved_game_file_path_linux(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_TRUE(build_saved_game_file_path(USER_DATA_PLATFORM_LINUX,
                                           "/home/ana", path, sizeof path));
    ASSERT_TRUE(strcmp(path, LINUX_SAVED_PATH) == 0);
}

static void test_build_saved_game_file_path_macos(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_TRUE(build_saved_game_file_path(USER_DATA_PLATFORM_MACOS,
                                           "/Users/ana", path, sizeof path));
    ASSERT_TRUE(strcmp(path, MACOS_SAVED_PATH) == 0);
}

static void test_build_saved_game_file_path_windows(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_TRUE(build_saved_game_file_path(USER_DATA_PLATFORM_WINDOWS,
                                           "C:\\Users\\ana\\AppData\\Local",
                                           path, sizeof path));
    ASSERT_TRUE(strcmp(path, WINDOWS_SAVED_PATH) == 0);
}

static void test_build_saved_game_file_path_base_with_trailing_separator(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_TRUE(build_saved_game_file_path(USER_DATA_PLATFORM_LINUX,
                                           "/home/ana/", path, sizeof path));
    ASSERT_TRUE(strcmp(path, LINUX_SAVED_PATH) == 0);
    ASSERT_TRUE(build_saved_game_file_path(USER_DATA_PLATFORM_MACOS,
                                           "/Users/ana/", path, sizeof path));
    ASSERT_TRUE(strcmp(path, MACOS_SAVED_PATH) == 0);
    ASSERT_TRUE(build_saved_game_file_path(USER_DATA_PLATFORM_WINDOWS,
                                           "C:\\x\\", path, sizeof path));
    ASSERT_TRUE(strcmp(path, "C:\\x\\Tetonor\\saved_game.txt") == 0);
}

static void test_build_saved_game_file_path_rejects_missing_base(void)
{
    const UserDataPlatform platforms[] = {USER_DATA_PLATFORM_LINUX,
                                          USER_DATA_PLATFORM_MACOS,
                                          USER_DATA_PLATFORM_WINDOWS};
    for (size_t i = 0; i < sizeof platforms / sizeof platforms[0]; i++)
    {
        char path[TEST_PATH_BUFFER_SIZE];
        memset(path, TEST_CANARY, sizeof path);
        ASSERT_FALSE(
            build_saved_game_file_path(platforms[i], NULL, path, sizeof path));
        ASSERT_EQ_INT(path[0], '\0');
        memset(path, TEST_CANARY, sizeof path);
        ASSERT_FALSE(
            build_saved_game_file_path(platforms[i], "", path, sizeof path));
        ASSERT_EQ_INT(path[0], '\0');
    }
}

static void test_build_saved_game_file_path_rejects_small_buffer(void)
{
    const struct
    {
        UserDataPlatform platform;
        const char* base;
        const char* expected;
    } cases[] = {
        {USER_DATA_PLATFORM_LINUX, "/home/ana", LINUX_SAVED_PATH},
        {USER_DATA_PLATFORM_MACOS, "/Users/ana", MACOS_SAVED_PATH},
        {USER_DATA_PLATFORM_WINDOWS, "C:\\Users\\ana\\AppData\\Local",
         WINDOWS_SAVED_PATH},
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++)
    {
        char path[TEST_PATH_BUFFER_SIZE];
        size_t length = strlen(cases[i].expected);
        memset(path, TEST_CANARY, sizeof path);
        ASSERT_FALSE(build_saved_game_file_path(cases[i].platform,
                                                cases[i].base, path, length));
        ASSERT_EQ_INT(path[0], '\0');
        ASSERT_TRUE(build_saved_game_file_path(cases[i].platform, cases[i].base,
                                               path, length + 1));
        ASSERT_TRUE(strcmp(path, cases[i].expected) == 0);
    }
}

static void test_build_saved_game_file_path_rejects_empty_buffer(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    memset(path, TEST_CANARY, sizeof path);
    ASSERT_FALSE(build_saved_game_file_path(USER_DATA_PLATFORM_LINUX,
                                            "/home/ana", path, 0));
    ASSERT_EQ_INT(path[0], TEST_CANARY);
}

static void test_saved_game_file_path_is_beside_the_statistics_file(void)
{
    const struct
    {
        UserDataPlatform platform;
        const char* base;
    } cases[] = {
        {USER_DATA_PLATFORM_LINUX, "/home/ana"},
        {USER_DATA_PLATFORM_MACOS, "/Users/ana"},
        {USER_DATA_PLATFORM_WINDOWS, "C:\\Users\\ana\\AppData\\Local"},
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++)
    {
        char stats[TEST_PATH_BUFFER_SIZE];
        char saved[TEST_PATH_BUFFER_SIZE];
        ASSERT_TRUE(build_stats_file_path(cases[i].platform, cases[i].base,
                                          stats, sizeof stats));
        ASSERT_TRUE(build_saved_game_file_path(cases[i].platform, cases[i].base,
                                               saved, sizeof saved));
        size_t directory = strlen(stats) - strlen("statistics.txt");
        ASSERT_EQ_INT(strlen(saved), directory + strlen("saved_game.txt"));
        ASSERT_EQ_INT(strncmp(stats, saved, directory), 0);
        ASSERT_TRUE(strcmp(saved + directory, "saved_game.txt") == 0);
    }
}

static void test_prepare_saved_game_file_path_creates_directories(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_TRUE(prepare_saved_game_file_path(current_user_data_platform(),
                                             TEST_TMP_DIR, path, sizeof path));
    FILE* file = fopen(path, "wb");
    ASSERT_NOT_NULL(file);
    fclose(file);
    remove(path);
}

static void test_prepare_saved_game_file_path_matches_the_built_path(void)
{
    char prepared[TEST_PATH_BUFFER_SIZE];
    char built[TEST_PATH_BUFFER_SIZE];
    UserDataPlatform platform = current_user_data_platform();
    ASSERT_TRUE(prepare_saved_game_file_path(platform, TEST_TMP_DIR, prepared,
                                             sizeof prepared));
    ASSERT_TRUE(build_saved_game_file_path(platform, TEST_TMP_DIR, built,
                                           sizeof built));
    ASSERT_TRUE(strcmp(prepared, built) == 0);
}

static void test_prepare_saved_game_file_path_is_idempotent(void)
{
    char first[TEST_PATH_BUFFER_SIZE];
    char second[TEST_PATH_BUFFER_SIZE];
    UserDataPlatform platform = current_user_data_platform();
    ASSERT_TRUE(prepare_saved_game_file_path(platform, TEST_TMP_DIR, first,
                                             sizeof first));
    ASSERT_TRUE(prepare_saved_game_file_path(platform, TEST_TMP_DIR, second,
                                             sizeof second));
    ASSERT_TRUE(strcmp(first, second) == 0);
}

static void test_prepare_saved_game_file_path_shares_the_stats_directory(void)
{
    char saved[TEST_PATH_BUFFER_SIZE];
    char stats[TEST_PATH_BUFFER_SIZE];
    UserDataPlatform platform = current_user_data_platform();
    ASSERT_TRUE(prepare_saved_game_file_path(platform, TEST_TMP_DIR, saved,
                                             sizeof saved));
    ASSERT_TRUE(
        prepare_stats_file_path(platform, TEST_TMP_DIR, stats, sizeof stats));
    size_t directory = strlen(stats) - strlen("statistics.txt");
    ASSERT_EQ_INT(strncmp(saved, stats, directory), 0);
    FILE* file = fopen(saved, "wb");
    ASSERT_NOT_NULL(file);
    fclose(file);
    remove(saved);
}

static void test_prepare_saved_game_file_path_keeps_existing_files(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    UserDataPlatform platform = current_user_data_platform();
    ASSERT_TRUE(prepare_saved_game_file_path(platform, TEST_TMP_DIR, path,
                                             sizeof path));
    FILE* file = fopen(path, "wb");
    ASSERT_NOT_NULL(file);
    fputs("kept", file);
    fclose(file);
    char again[TEST_PATH_BUFFER_SIZE];
    bool prepared = prepare_saved_game_file_path(platform, TEST_TMP_DIR, again,
                                                 sizeof again);
    char content[8] = {0};
    file            = fopen(path, "rb");
    ASSERT_NOT_NULL(file);
    size_t length = fread(content, 1, sizeof content - 1, file);
    fclose(file);
    remove(path);
    ASSERT_TRUE(prepared);
    ASSERT_EQ_INT(length, 4);
    ASSERT_TRUE(strcmp(content, "kept") == 0);
}

static void test_prepare_saved_game_file_path_fails_under_a_file(void)
{
    const char* base = TEST_TMP_DIR "/platform_saved_base_is_a_file.txt";
    FILE* file       = fopen(base, "wb");
    ASSERT_NOT_NULL(file);
    fclose(file);
    char path[TEST_PATH_BUFFER_SIZE];
    bool prepared = prepare_saved_game_file_path(current_user_data_platform(),
                                                 base, path, sizeof path);
    remove(base);
    ASSERT_FALSE(prepared);
}

static void test_prepare_saved_game_file_path_does_not_create_the_base(void)
{
    const char* base = TEST_TMP_DIR "/platform_saved_missing_base";
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_FALSE(prepare_saved_game_file_path(current_user_data_platform(),
                                              base, path, sizeof path));
    FILE* probe =
        fopen(TEST_TMP_DIR "/platform_saved_missing_base/probe", "wb");
    if (probe != NULL)
    {
        fclose(probe);
        remove(TEST_TMP_DIR "/platform_saved_missing_base/probe");
    }
    ASSERT_NULL(probe);
}

static void test_prepare_saved_game_file_path_rejects_missing_base(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    memset(path, TEST_CANARY, sizeof path);
    ASSERT_FALSE(prepare_saved_game_file_path(current_user_data_platform(),
                                              NULL, path, sizeof path));
    ASSERT_FALSE(prepare_saved_game_file_path(current_user_data_platform(), "",
                                              path, sizeof path));
}

static void test_prepare_saved_game_file_path_rejects_small_buffer(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_FALSE(prepare_saved_game_file_path(current_user_data_platform(),
                                              TEST_TMP_DIR, path, 8));
}

static void test_stats_file_paths_are_unchanged_by_the_saved_game_file(void)
{
    char path[TEST_PATH_BUFFER_SIZE];
    ASSERT_TRUE(build_stats_file_path(USER_DATA_PLATFORM_LINUX, "/home/ana",
                                      path, sizeof path));
    ASSERT_TRUE(strcmp(path, LINUX_PATH) == 0);
    ASSERT_TRUE(build_stats_file_path(USER_DATA_PLATFORM_MACOS, "/Users/ana",
                                      path, sizeof path));
    ASSERT_TRUE(strcmp(path, MACOS_PATH) == 0);
    ASSERT_TRUE(build_stats_file_path(USER_DATA_PLATFORM_WINDOWS,
                                      "C:\\Users\\ana\\AppData\\Local", path,
                                      sizeof path));
    ASSERT_TRUE(strcmp(path, WINDOWS_PATH) == 0);
}

int main(void)
{
    RUN_TEST(test_user_data_env_names);
    RUN_TEST(test_current_user_data_platform_is_known);
    RUN_TEST(test_build_stats_file_path_linux);
    RUN_TEST(test_build_stats_file_path_macos);
    RUN_TEST(test_build_stats_file_path_windows);
    RUN_TEST(test_build_stats_file_path_base_with_trailing_separator);
    RUN_TEST(test_build_stats_file_path_windows_base_with_forward_slash);
    RUN_TEST(test_build_stats_file_path_rejects_missing_base);
    RUN_TEST(test_build_stats_file_path_rejects_small_buffer);
    RUN_TEST(test_build_stats_file_path_rejects_empty_buffer);
    RUN_TEST(test_prepare_stats_file_path_creates_directories);
    RUN_TEST(test_prepare_stats_file_path_matches_build_stats_file_path);
    RUN_TEST(test_prepare_stats_file_path_is_idempotent);
    RUN_TEST(test_prepare_stats_file_path_keeps_existing_files);
    RUN_TEST(test_prepare_stats_file_path_fails_under_a_file);
    RUN_TEST(test_prepare_stats_file_path_does_not_create_the_base);
    RUN_TEST(test_prepare_stats_file_path_rejects_missing_base);
    RUN_TEST(test_prepare_stats_file_path_rejects_small_buffer);
    RUN_TEST(test_build_user_data_file_path_config_on_every_platform);
    RUN_TEST(test_build_user_data_file_path_config_shares_the_stats_directory);
    RUN_TEST(test_build_stats_file_path_equals_the_generic_function);
    RUN_TEST(test_build_user_data_file_path_base_with_trailing_separator);
    RUN_TEST(test_build_user_data_file_path_rejects_missing_file_name);
    RUN_TEST(test_build_user_data_file_path_rejects_missing_base);
    RUN_TEST(test_build_user_data_file_path_rejects_small_buffer);
    RUN_TEST(test_build_user_data_file_path_rejects_empty_buffer);
    RUN_TEST(test_prepare_user_data_file_path_creates_the_directory);
    RUN_TEST(test_prepare_user_data_file_path_matches_the_build_function);
    RUN_TEST(test_prepare_user_data_file_path_shares_the_stats_directory);
    RUN_TEST(test_prepare_user_data_file_path_rejects_bad_arguments);
    RUN_TEST(test_prepare_user_data_file_path_fails_under_a_file);
    RUN_TEST(test_build_saved_game_file_path_linux);
    RUN_TEST(test_build_saved_game_file_path_macos);
    RUN_TEST(test_build_saved_game_file_path_windows);
    RUN_TEST(test_build_saved_game_file_path_base_with_trailing_separator);
    RUN_TEST(test_build_saved_game_file_path_rejects_missing_base);
    RUN_TEST(test_build_saved_game_file_path_rejects_small_buffer);
    RUN_TEST(test_build_saved_game_file_path_rejects_empty_buffer);
    RUN_TEST(test_saved_game_file_path_is_beside_the_statistics_file);
    RUN_TEST(test_prepare_saved_game_file_path_creates_directories);
    RUN_TEST(test_prepare_saved_game_file_path_matches_the_built_path);
    RUN_TEST(test_prepare_saved_game_file_path_is_idempotent);
    RUN_TEST(test_prepare_saved_game_file_path_shares_the_stats_directory);
    RUN_TEST(test_prepare_saved_game_file_path_keeps_existing_files);
    RUN_TEST(test_prepare_saved_game_file_path_fails_under_a_file);
    RUN_TEST(test_prepare_saved_game_file_path_does_not_create_the_base);
    RUN_TEST(test_prepare_saved_game_file_path_rejects_missing_base);
    RUN_TEST(test_prepare_saved_game_file_path_rejects_small_buffer);
    RUN_TEST(test_stats_file_paths_are_unchanged_by_the_saved_game_file);
    return test_harness_result();
}
