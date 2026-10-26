#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../test_harness.h"
#include "../../src/storage/config_store.h"

/* make test recreates this directory before running any test binary. */
#define TEST_TMP_DIR "build/tests/tmp"
#define TEST_PATH(name) (TEST_TMP_DIR "/" name)
#define TEST_TEXT_BUFFER_SIZE 128
#define TEST_FAR_THEME 99
#define TEST_GARBAGE_BYTE 0x5a

#define CLASSIC_TEXT "TETONOR-CONFIG 1\ntheme classic\n"
#define LIGHT_TEXT "TETONOR-CONFIG 1\ntheme light\n"

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

static bool write_text(const char* path, const char* text)
{
    return write_bytes(path, text, strlen(text));
}

static void remove_with_temporary(const char* path)
{
    char temporary[TETONOR_CONFIG_PATH_MAX + 8];
    remove(path);
    snprintf(temporary, sizeof temporary, "%s.tmp", path);
    remove(temporary);
}

static bool temporary_file_exists(const char* path)
{
    char temporary[TETONOR_CONFIG_PATH_MAX + 8];
    snprintf(temporary, sizeof temporary, "%s.tmp", path);
    return file_exists(temporary);
}

static size_t read_file(const char* path, char* out, size_t capacity)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL)
    {
        return 0;
    }
    size_t length = fread(out, 1, capacity - 1, file);
    fclose(file);
    out[length] = '\0';
    return length;
}

static void make_too_long_path(char* path, size_t size)
{
    memset(path, 'a', size);
    memcpy(path, TEST_TMP_DIR "/", sizeof(TEST_TMP_DIR "/") - 1);
    path[size - 1] = '\0';
}

static void test_config_constants_are_pinned(void)
{
    ASSERT_TRUE(strcmp(TETONOR_CONFIG_FILE_NAME, "config.txt") == 0);
    ASSERT_EQ_INT(TETONOR_CONFIG_TEXT_MAX, 64);
    ASSERT_EQ_INT(TETONOR_CONFIG_PATH_MAX, TETONOR_TEXT_FILE_PATH_MAX);
}

static void test_serialize_config_writes_the_pinned_text(void)
{
    char text[TEST_TEXT_BUFFER_SIZE];
    ASSERT_TRUE(serialize_config(PALETTE_THEME_CLASSIC, text, sizeof text));
    ASSERT_TRUE(strcmp(text, CLASSIC_TEXT) == 0);
    ASSERT_TRUE(serialize_config(PALETTE_THEME_LIGHT, text, sizeof text));
    ASSERT_TRUE(strcmp(text, LIGHT_TEXT) == 0);
}

static void test_serialize_config_text_fits_the_documented_maximum(void)
{
    char text[TEST_TEXT_BUFFER_SIZE];
    for (int theme = 0; theme < PALETTE_THEME_COUNT; theme++)
    {
        ASSERT_TRUE(serialize_config(theme, text, sizeof text));
        ASSERT_TRUE(strlen(text) <= TETONOR_CONFIG_TEXT_MAX);
    }
}

static void test_serialize_config_rejects_a_small_buffer(void)
{
    char text[TEST_TEXT_BUFFER_SIZE];
    size_t length = strlen(LIGHT_TEXT);
    ASSERT_FALSE(serialize_config(PALETTE_THEME_LIGHT, text, length));
    ASSERT_FALSE(serialize_config(PALETTE_THEME_LIGHT, text, 1));
    ASSERT_FALSE(serialize_config(PALETTE_THEME_LIGHT, text, 0));
    ASSERT_TRUE(serialize_config(PALETTE_THEME_LIGHT, text, length + 1));
    ASSERT_TRUE(strcmp(text, LIGHT_TEXT) == 0);
}

static void test_serialize_config_rejects_an_invalid_theme(void)
{
    char text[TEST_TEXT_BUFFER_SIZE];
    ASSERT_FALSE(serialize_config(-1, text, sizeof text));
    ASSERT_FALSE(serialize_config(PALETTE_THEME_COUNT, text, sizeof text));
    ASSERT_FALSE(serialize_config(TEST_FAR_THEME, text, sizeof text));
}

static void test_parse_config_accepts_valid_texts(void)
{
    const struct
    {
        const char* text;
        PaletteTheme expected;
    } cases[] = {
        {"TETONOR-CONFIG 1\ntheme classic\n", PALETTE_THEME_CLASSIC},
        {"TETONOR-CONFIG 1\ntheme light\n", PALETTE_THEME_LIGHT},
        {"TETONOR-CONFIG 1\r\ntheme classic\r\n", PALETTE_THEME_CLASSIC},
        {"TETONOR-CONFIG 1\r\ntheme light\r\n", PALETTE_THEME_LIGHT},
        {"TETONOR-CONFIG 1\ntheme light", PALETTE_THEME_LIGHT},
        {"TETONOR-CONFIG 1\ntheme classic", PALETTE_THEME_CLASSIC},
        {"TETONOR-CONFIG 1\r\ntheme light", PALETTE_THEME_LIGHT},
        {"TETONOR-CONFIG 1\ntheme light\r\n", PALETTE_THEME_LIGHT},
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++)
    {
        PaletteTheme theme = cases[i].expected == PALETTE_THEME_LIGHT
                                 ? PALETTE_THEME_CLASSIC
                                 : PALETTE_THEME_LIGHT;
        ASSERT_TRUE(parse_config(cases[i].text, strlen(cases[i].text), &theme));
        ASSERT_EQ_INT(theme, cases[i].expected);
    }
}

static void test_parse_config_uses_the_length_not_a_terminator(void)
{
    char text[TEST_TEXT_BUFFER_SIZE];
    memset(text, 'x', sizeof text);
    memcpy(text, LIGHT_TEXT, strlen(LIGHT_TEXT));
    PaletteTheme theme = PALETTE_THEME_CLASSIC;
    ASSERT_TRUE(parse_config(text, strlen(LIGHT_TEXT), &theme));
    ASSERT_EQ_INT(theme, PALETTE_THEME_LIGHT);
    theme = PALETTE_THEME_CLASSIC;
    ASSERT_FALSE(parse_config(text, strlen(LIGHT_TEXT) + 1, &theme));
    ASSERT_EQ_INT(theme, PALETTE_THEME_CLASSIC);
}

static void test_parse_config_accepts_what_serialize_config_writes(void)
{
    for (int expected = 0; expected < PALETTE_THEME_COUNT; expected++)
    {
        char text[TEST_TEXT_BUFFER_SIZE];
        ASSERT_TRUE(serialize_config(expected, text, sizeof text));
        PaletteTheme theme =
            (PaletteTheme)((expected + 1) % PALETTE_THEME_COUNT);
        ASSERT_TRUE(parse_config(text, strlen(text), &theme));
        ASSERT_EQ_INT(theme, expected);
    }
}

static void test_parse_config_rejects_malformed_texts(void)
{
    const char* const rejected[] = {
        "",
        "\n",
        "TETONOR-CONFIG 1",
        "TETONOR-CONFIG 1\n",
        "TETONOR-CONFIG 1\r\n",
        "TETONOR-CONFIG 2\ntheme light\n",
        "TETONOR-CONFIG 0\ntheme light\n",
        "TETONOR-CONFIG 10\ntheme light\n",
        "TETONOR-CONFIG\ntheme light\n",
        "TETONOR-CONFIG  1\ntheme light\n",
        "TETONOR-CONFIG 1 \ntheme light\n",
        "tetonor-config 1\ntheme light\n",
        "TETONOR-STATS 1\ntheme light\n",
        "theme light\n",
        "TETONOR-CONFIG 1\nclassic\n",
        "TETONOR-CONFIG 1\ntheme\n",
        "TETONOR-CONFIG 1\ntheme \n",
        "TETONOR-CONFIG 1\nTheme light\n",
        "TETONOR-CONFIG 1\nthemes light\n",
        "TETONOR-CONFIG 1\ntheme dark\n",
        "TETONOR-CONFIG 1\ntheme Light\n",
        "TETONOR-CONFIG 1\ntheme LIGHT\n",
        "TETONOR-CONFIG 1\ntheme lig\n",
        "TETONOR-CONFIG 1\ntheme lightx\n",
        "TETONOR-CONFIG 1\ntheme  light\n",
        "TETONOR-CONFIG 1\ntheme light \n",
        "TETONOR-CONFIG 1\n theme light\n",
        "TETONOR-CONFIG 1\ntheme\tlight\n",
        "TETONOR-CONFIG 1\ntheme light\nextra\n",
        "TETONOR-CONFIG 1\ntheme light\nextra",
        "TETONOR-CONFIG 1\ntheme light\n\n",
        "TETONOR-CONFIG 1\ntheme light\ntheme classic\n",
        "TETONOR-CONFIG 1\n\ntheme light\n",
        "\nTETONOR-CONFIG 1\ntheme light\n",
        "TETONOR-CONFIG 1\rtheme light\n",
        "TETONOR-CONFIG 1\ntheme light\r",
        "TETONOR-CONFIG 1\ntheme light\r\r\n",
        "TETONOR-CONFIG 1\n\rtheme light\n",
        "TETONOR-CONFIG 1\ntheme 0\n",
        "TETONOR-CONFIG 1\ntheme 1\n",
        "TETONOR-CONFIG 1\nlanguage en\n",
    };
    for (size_t i = 0; i < sizeof rejected / sizeof rejected[0]; i++)
    {
        PaletteTheme theme = PALETTE_THEME_LIGHT;
        if (parse_config(rejected[i], strlen(rejected[i]), &theme))
        {
            TEST_HARNESS_FAIL("accepted: case %d", (int)i);
        }
        ASSERT_EQ_INT(theme, PALETTE_THEME_LIGHT);
        theme = PALETTE_THEME_CLASSIC;
        parse_config(rejected[i], strlen(rejected[i]), &theme);
        ASSERT_EQ_INT(theme, PALETTE_THEME_CLASSIC);
    }
}

static void test_parse_config_rejects_an_embedded_nul_byte(void)
{
    const char with_nul[] = "TETONOR-CONFIG 1\ntheme li\0ht\n";
    PaletteTheme theme    = PALETTE_THEME_CLASSIC;
    ASSERT_FALSE(parse_config(with_nul, sizeof with_nul - 1, &theme));
    ASSERT_EQ_INT(theme, PALETTE_THEME_CLASSIC);
    const char nul_after[] = "TETONOR-CONFIG 1\ntheme light\n\0";
    ASSERT_FALSE(parse_config(nul_after, sizeof nul_after - 1, &theme));
    ASSERT_EQ_INT(theme, PALETTE_THEME_CLASSIC);
    const char nul_in_header[] = "TETONOR-CONFIG\0001\ntheme light\n";
    ASSERT_FALSE(parse_config(nul_in_header, sizeof nul_in_header - 1, &theme));
    ASSERT_EQ_INT(theme, PALETTE_THEME_CLASSIC);
}

static void test_parse_config_rejects_oversized_texts(void)
{
    char text[TETONOR_CONFIG_TEXT_MAX + 16];
    memset(text, '\n', sizeof text);
    memcpy(text, LIGHT_TEXT, strlen(LIGHT_TEXT));
    PaletteTheme theme = PALETTE_THEME_CLASSIC;
    ASSERT_FALSE(parse_config(text, TETONOR_CONFIG_TEXT_MAX + 1, &theme));
    ASSERT_FALSE(parse_config(text, sizeof text, &theme));
    ASSERT_EQ_INT(theme, PALETTE_THEME_CLASSIC);
}

static void test_save_then_load_config_round_trips(void)
{
    const char* path = TEST_PATH("config_round_trip.txt");
    for (int expected = 0; expected < PALETTE_THEME_COUNT; expected++)
    {
        remove_with_temporary(path);
        ASSERT_TRUE(save_config_file(path, expected));
        ASSERT_FALSE(temporary_file_exists(path));
        PaletteTheme loaded =
            (PaletteTheme)((expected + 1) % PALETTE_THEME_COUNT);
        ConfigLoadResult result = load_config_file(path, &loaded);
        remove_with_temporary(path);
        ASSERT_EQ_INT(result, CONFIG_LOAD_LOADED);
        ASSERT_EQ_INT(loaded, expected);
    }
}

static void test_save_config_file_writes_the_pinned_text(void)
{
    const char* path = TEST_PATH("config_pinned_text.txt");
    remove_with_temporary(path);
    ASSERT_TRUE(save_config_file(path, PALETTE_THEME_LIGHT));
    char text[TEST_TEXT_BUFFER_SIZE];
    read_file(path, text, sizeof text);
    remove_with_temporary(path);
    ASSERT_TRUE(strcmp(text, LIGHT_TEXT) == 0);
}

static void test_save_config_file_overwrites_an_existing_file(void)
{
    const char* path = TEST_PATH("config_overwrite.txt");
    remove_with_temporary(path);
    ASSERT_TRUE(save_config_file(path, PALETTE_THEME_LIGHT));
    ASSERT_TRUE(save_config_file(path, PALETTE_THEME_CLASSIC));
    char text[TEST_TEXT_BUFFER_SIZE];
    read_file(path, text, sizeof text);
    remove_with_temporary(path);
    ASSERT_TRUE(strcmp(text, CLASSIC_TEXT) == 0);
}

static void test_save_config_file_replaces_a_corrupted_file(void)
{
    const char* path = TEST_PATH("config_replace_corrupted.txt");
    ASSERT_TRUE(write_text(path, "not a config file at all, just junk"));
    ASSERT_TRUE(save_config_file(path, PALETTE_THEME_LIGHT));
    PaletteTheme loaded     = PALETTE_THEME_CLASSIC;
    ConfigLoadResult result = load_config_file(path, &loaded);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, CONFIG_LOAD_LOADED);
    ASSERT_EQ_INT(loaded, PALETTE_THEME_LIGHT);
}

static void test_save_config_file_rejects_an_invalid_theme(void)
{
    const char* path = TEST_PATH("config_invalid_theme.txt");
    remove_with_temporary(path);
    ASSERT_FALSE(save_config_file(path, -1));
    ASSERT_FALSE(save_config_file(path, PALETTE_THEME_COUNT));
    ASSERT_FALSE(file_exists(path));
    ASSERT_FALSE(temporary_file_exists(path));
}

static void test_save_config_file_write_failure_returns_false(void)
{
    const char* path = TEST_PATH("no_such_directory/config.txt");
    ASSERT_FALSE(save_config_file(path, PALETTE_THEME_LIGHT));
    ASSERT_FALSE(file_exists(path));
    ASSERT_FALSE(temporary_file_exists(path));
}

static void test_save_config_file_rejects_too_long_path(void)
{
    char path[TETONOR_CONFIG_PATH_MAX + 1];
    make_too_long_path(path, sizeof path);
    ASSERT_FALSE(save_config_file(path, PALETTE_THEME_LIGHT));
    ASSERT_FALSE(file_exists(path));
}

static void test_load_config_missing_file_is_default(void)
{
    const char* path = TEST_PATH("config_missing.txt");
    remove_with_temporary(path);
    PaletteTheme theme      = PALETTE_THEME_LIGHT;
    ConfigLoadResult result = load_config_file(path, &theme);
    ASSERT_EQ_INT(result, CONFIG_LOAD_MISSING);
    ASSERT_EQ_INT(theme, TETONOR_DEFAULT_PALETTE_THEME);
}

static void test_load_config_empty_corrupt_unknown_are_default(void)
{
    const char* path          = TEST_PATH("config_unreadable.txt");
    const char* const texts[] = {
        "",
        "garbage",
        "TETONOR-CONFIG 1\n",
        "TETONOR-CONFIG 2\ntheme light\n",
        "TETONOR-CONFIG 1\ntheme dark\n",
        "TETONOR-CONFIG 1\ntheme light\nextra\n",
        "TETONOR-STATS 1\neasy 0 0 0\nmedium 0 0 0\nhard 0 0 0\n",
    };
    for (size_t i = 0; i < sizeof texts / sizeof texts[0]; i++)
    {
        ASSERT_TRUE(write_text(path, texts[i]));
        PaletteTheme theme      = PALETTE_THEME_LIGHT;
        ConfigLoadResult result = load_config_file(path, &theme);
        remove_with_temporary(path);
        ASSERT_EQ_INT(result, CONFIG_LOAD_CORRUPTED);
        ASSERT_EQ_INT(theme, TETONOR_DEFAULT_PALETTE_THEME);
    }
}

static void test_load_config_binary_garbage_is_default(void)
{
    const char* path = TEST_PATH("config_binary.txt");
    unsigned char bytes[TETONOR_CONFIG_TEXT_MAX];
    for (size_t i = 0; i < sizeof bytes; i++)
    {
        bytes[i] = (unsigned char)(i * 37u + 11u);
    }
    ASSERT_TRUE(write_bytes(path, bytes, sizeof bytes));
    PaletteTheme theme      = PALETTE_THEME_LIGHT;
    ConfigLoadResult result = load_config_file(path, &theme);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, CONFIG_LOAD_CORRUPTED);
    ASSERT_EQ_INT(theme, TETONOR_DEFAULT_PALETTE_THEME);
}

static void test_load_config_accepts_crlf_file(void)
{
    const char* path = TEST_PATH("config_crlf.txt");
    ASSERT_TRUE(write_text(path, "TETONOR-CONFIG 1\r\ntheme light\r\n"));
    PaletteTheme theme      = PALETTE_THEME_CLASSIC;
    ConfigLoadResult result = load_config_file(path, &theme);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, CONFIG_LOAD_LOADED);
    ASSERT_EQ_INT(theme, PALETTE_THEME_LIGHT);
}

static void test_load_config_oversized_is_corrupted(void)
{
    const char* path = TEST_PATH("config_oversized.txt");
    char text[TETONOR_CONFIG_TEXT_MAX + 1];
    memset(text, '\n', sizeof text);
    memcpy(text, LIGHT_TEXT, strlen(LIGHT_TEXT));
    ASSERT_TRUE(write_bytes(path, text, sizeof text));
    PaletteTheme theme      = PALETTE_THEME_LIGHT;
    ConfigLoadResult result = load_config_file(path, &theme);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, CONFIG_LOAD_CORRUPTED);
    ASSERT_EQ_INT(theme, TETONOR_DEFAULT_PALETTE_THEME);
}

static void test_load_config_huge_file_is_corrupted(void)
{
    const char* path = TEST_PATH("config_huge.txt");
    char text[TETONOR_CONFIG_TEXT_MAX * 20];
    memset(text, 'a', sizeof text);
    ASSERT_TRUE(write_bytes(path, text, sizeof text));
    PaletteTheme theme      = PALETTE_THEME_LIGHT;
    ConfigLoadResult result = load_config_file(path, &theme);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, CONFIG_LOAD_CORRUPTED);
    ASSERT_EQ_INT(theme, TETONOR_DEFAULT_PALETTE_THEME);
}

static void test_load_config_never_returns_disabled(void)
{
    const char* path = TEST_PATH("config_never_disabled.txt");
    remove_with_temporary(path);
    PaletteTheme theme = PALETTE_THEME_CLASSIC;
    ASSERT_TRUE(load_config_file(path, &theme) != CONFIG_LOAD_DISABLED);
    ASSERT_TRUE(write_text(path, "junk"));
    ASSERT_TRUE(load_config_file(path, &theme) != CONFIG_LOAD_DISABLED);
    remove_with_temporary(path);
}

static void test_init_config_store_disabled_paths(void)
{
    char too_long[TETONOR_CONFIG_PATH_MAX + 1];
    make_too_long_path(too_long, sizeof too_long);
    const char* paths[] = {NULL, "", too_long};
    for (size_t i = 0; i < sizeof paths / sizeof paths[0]; i++)
    {
        ConfigStore store;
        memset(&store, TEST_GARBAGE_BYTE, sizeof store);
        ASSERT_EQ_INT(init_config_store(&store, paths[i]),
                      CONFIG_LOAD_DISABLED);
        ASSERT_EQ_INT(store.theme, TETONOR_DEFAULT_PALETTE_THEME);
        ASSERT_FALSE(store.persistent);
    }
    ASSERT_FALSE(file_exists(too_long));
}

static void test_init_config_store_missing_file_is_default_and_persists(void)
{
    const char* path = TEST_PATH("config_store_missing.txt");
    remove_with_temporary(path);
    ConfigStore store;
    memset(&store, TEST_GARBAGE_BYTE, sizeof store);
    ASSERT_EQ_INT(init_config_store(&store, path), CONFIG_LOAD_MISSING);
    ASSERT_EQ_INT(store.theme, TETONOR_DEFAULT_PALETTE_THEME);
    ASSERT_TRUE(store.persistent);
    ASSERT_TRUE(strcmp(store.path, path) == 0);
    ASSERT_FALSE(file_exists(path));
}

static void test_init_config_store_loads_saved_theme(void)
{
    const char* path = TEST_PATH("config_store_loads.txt");
    remove_with_temporary(path);
    ASSERT_TRUE(save_config_file(path, PALETTE_THEME_LIGHT));
    ConfigStore store;
    ConfigLoadResult result = init_config_store(&store, path);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, CONFIG_LOAD_LOADED);
    ASSERT_EQ_INT(store.theme, PALETTE_THEME_LIGHT);
    ASSERT_TRUE(store.persistent);
    ASSERT_TRUE(strcmp(store.path, path) == 0);
}

static void test_init_config_store_corrupted_file_is_default_and_persists(void)
{
    const char* path = TEST_PATH("config_store_corrupted.txt");
    ASSERT_TRUE(write_text(path, "TETONOR-CONFIG 1\ntheme dark\n"));
    ConfigStore store;
    memset(&store, TEST_GARBAGE_BYTE, sizeof store);
    ConfigLoadResult result = init_config_store(&store, path);
    ASSERT_EQ_INT(result, CONFIG_LOAD_CORRUPTED);
    ASSERT_EQ_INT(store.theme, TETONOR_DEFAULT_PALETTE_THEME);
    ASSERT_TRUE(store.persistent);
    ASSERT_TRUE(set_config_store_theme(&store, PALETTE_THEME_LIGHT));
    PaletteTheme reloaded  = PALETTE_THEME_CLASSIC;
    ConfigLoadResult again = load_config_file(path, &reloaded);
    remove_with_temporary(path);
    ASSERT_EQ_INT(again, CONFIG_LOAD_LOADED);
    ASSERT_EQ_INT(reloaded, PALETTE_THEME_LIGHT);
}

static void test_init_config_store_accepts_the_longest_path(void)
{
    char path[TETONOR_CONFIG_PATH_MAX];
    make_too_long_path(path, sizeof path);
    ConfigStore store;
    ASSERT_TRUE(init_config_store(&store, path) != CONFIG_LOAD_DISABLED);
    ASSERT_TRUE(store.persistent);
    ASSERT_TRUE(strcmp(store.path, path) == 0);
}

static void test_set_config_store_theme_saves_when_persistent(void)
{
    const char* path = TEST_PATH("config_store_set.txt");
    remove_with_temporary(path);
    ConfigStore store;
    init_config_store(&store, path);
    ASSERT_TRUE(set_config_store_theme(&store, PALETTE_THEME_LIGHT));
    ASSERT_EQ_INT(store.theme, PALETTE_THEME_LIGHT);
    ASSERT_FALSE(temporary_file_exists(path));
    ConfigStore next_session;
    ConfigLoadResult result = init_config_store(&next_session, path);
    remove_with_temporary(path);
    ASSERT_EQ_INT(result, CONFIG_LOAD_LOADED);
    ASSERT_EQ_INT(next_session.theme, PALETTE_THEME_LIGHT);
}

static void test_set_config_store_theme_can_return_to_classic(void)
{
    const char* path = TEST_PATH("config_store_back.txt");
    remove_with_temporary(path);
    ConfigStore store;
    init_config_store(&store, path);
    ASSERT_TRUE(set_config_store_theme(&store, PALETTE_THEME_LIGHT));
    ASSERT_TRUE(set_config_store_theme(&store, PALETTE_THEME_CLASSIC));
    char text[TEST_TEXT_BUFFER_SIZE];
    read_file(path, text, sizeof text);
    remove_with_temporary(path);
    ASSERT_EQ_INT(store.theme, PALETTE_THEME_CLASSIC);
    ASSERT_TRUE(strcmp(text, CLASSIC_TEXT) == 0);
}

static void test_set_config_store_theme_write_failure_keeps_theme(void)
{
    const char* path = TEST_PATH("no_such_directory/config_store.txt");
    ConfigStore store;
    ASSERT_EQ_INT(init_config_store(&store, path), CONFIG_LOAD_MISSING);
    ASSERT_TRUE(store.persistent);
    ASSERT_FALSE(set_config_store_theme(&store, PALETTE_THEME_LIGHT));
    ASSERT_EQ_INT(store.theme, PALETTE_THEME_LIGHT);
    ASSERT_FALSE(file_exists(path));
    ASSERT_FALSE(set_config_store_theme(&store, PALETTE_THEME_CLASSIC));
    ASSERT_EQ_INT(store.theme, PALETTE_THEME_CLASSIC);
}

static void test_set_config_store_theme_rejects_invalid(void)
{
    const char* path = TEST_PATH("config_store_invalid.txt");
    remove_with_temporary(path);
    ConfigStore store;
    init_config_store(&store, path);
    ASSERT_FALSE(set_config_store_theme(&store, -1));
    ASSERT_FALSE(set_config_store_theme(&store, PALETTE_THEME_COUNT));
    ASSERT_FALSE(set_config_store_theme(&store, TEST_FAR_THEME));
    ASSERT_EQ_INT(store.theme, TETONOR_DEFAULT_PALETTE_THEME);
    ASSERT_FALSE(file_exists(path));

    ASSERT_TRUE(set_config_store_theme(&store, PALETTE_THEME_LIGHT));
    ASSERT_FALSE(set_config_store_theme(&store, PALETTE_THEME_COUNT));
    ASSERT_EQ_INT(store.theme, PALETTE_THEME_LIGHT);
    char text[TEST_TEXT_BUFFER_SIZE];
    read_file(path, text, sizeof text);
    remove_with_temporary(path);
    ASSERT_TRUE(strcmp(text, LIGHT_TEXT) == 0);
}

static void test_set_config_store_theme_memory_only_is_true(void)
{
    ConfigStore store;
    ASSERT_EQ_INT(init_config_store(&store, NULL), CONFIG_LOAD_DISABLED);
    ASSERT_TRUE(set_config_store_theme(&store, PALETTE_THEME_LIGHT));
    ASSERT_EQ_INT(store.theme, PALETTE_THEME_LIGHT);
    ASSERT_FALSE(set_config_store_theme(&store, -1));
    ASSERT_EQ_INT(store.theme, PALETTE_THEME_LIGHT);
}

int main(void)
{
    RUN_TEST(test_config_constants_are_pinned);
    RUN_TEST(test_serialize_config_writes_the_pinned_text);
    RUN_TEST(test_serialize_config_text_fits_the_documented_maximum);
    RUN_TEST(test_serialize_config_rejects_a_small_buffer);
    RUN_TEST(test_serialize_config_rejects_an_invalid_theme);
    RUN_TEST(test_parse_config_accepts_valid_texts);
    RUN_TEST(test_parse_config_uses_the_length_not_a_terminator);
    RUN_TEST(test_parse_config_accepts_what_serialize_config_writes);
    RUN_TEST(test_parse_config_rejects_malformed_texts);
    RUN_TEST(test_parse_config_rejects_an_embedded_nul_byte);
    RUN_TEST(test_parse_config_rejects_oversized_texts);
    RUN_TEST(test_save_then_load_config_round_trips);
    RUN_TEST(test_save_config_file_writes_the_pinned_text);
    RUN_TEST(test_save_config_file_overwrites_an_existing_file);
    RUN_TEST(test_save_config_file_replaces_a_corrupted_file);
    RUN_TEST(test_save_config_file_rejects_an_invalid_theme);
    RUN_TEST(test_save_config_file_write_failure_returns_false);
    RUN_TEST(test_save_config_file_rejects_too_long_path);
    RUN_TEST(test_load_config_missing_file_is_default);
    RUN_TEST(test_load_config_empty_corrupt_unknown_are_default);
    RUN_TEST(test_load_config_binary_garbage_is_default);
    RUN_TEST(test_load_config_accepts_crlf_file);
    RUN_TEST(test_load_config_oversized_is_corrupted);
    RUN_TEST(test_load_config_huge_file_is_corrupted);
    RUN_TEST(test_load_config_never_returns_disabled);
    RUN_TEST(test_init_config_store_disabled_paths);
    RUN_TEST(test_init_config_store_missing_file_is_default_and_persists);
    RUN_TEST(test_init_config_store_loads_saved_theme);
    RUN_TEST(test_init_config_store_corrupted_file_is_default_and_persists);
    RUN_TEST(test_init_config_store_accepts_the_longest_path);
    RUN_TEST(test_set_config_store_theme_saves_when_persistent);
    RUN_TEST(test_set_config_store_theme_can_return_to_classic);
    RUN_TEST(test_set_config_store_theme_write_failure_keeps_theme);
    RUN_TEST(test_set_config_store_theme_rejects_invalid);
    RUN_TEST(test_set_config_store_theme_memory_only_is_true);
    return test_harness_result();
}
