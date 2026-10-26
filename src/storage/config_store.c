#include <stdio.h>
#include <string.h>

#include "config_store.h"

static const char file_header[]                          = "TETONOR-CONFIG 1";
static const char theme_line_key[]                       = "theme ";

static const char* const theme_keys[PALETTE_THEME_COUNT] = {
    [PALETTE_THEME_CLASSIC] = "classic",
    [PALETTE_THEME_LIGHT]   = "light",
};

typedef struct
{
    const char* text;
    size_t length;
    size_t position;
} ConfigCursor;

static bool at_end(const ConfigCursor* cursor)
{
    return cursor->position == cursor->length;
}

/* Consumes expected when the text continues with exactly it. */
static bool expect_text(ConfigCursor* cursor, const char* expected)
{
    size_t length = strlen(expected);
    if (cursor->length - cursor->position < length ||
        memcmp(cursor->text + cursor->position, expected, length) != 0)
    {
        return false;
    }
    cursor->position += length;
    return true;
}

/* Consumes "\n" or "\r\n". */
static bool expect_line_end(ConfigCursor* cursor)
{
    return expect_text(cursor, "\n") || expect_text(cursor, "\r\n");
}

/* Consumes one theme key and returns its theme. */
static bool expect_theme_key(ConfigCursor* cursor, PaletteTheme* theme)
{
    for (int candidate = 0; candidate < PALETTE_THEME_COUNT; candidate++)
    {
        if (expect_text(cursor, theme_keys[candidate]))
        {
            *theme = (PaletteTheme)candidate;
            return true;
        }
    }
    return false;
}

bool serialize_config(int theme, char* out, size_t size)
{
    if (!is_palette_theme(theme))
    {
        return false;
    }
    int written = snprintf(out, size, "%s\n%s%s\n", file_header, theme_line_key,
                           theme_keys[theme]);
    return written >= 0 && (size_t)written < size;
}

bool parse_config(const char* text, size_t length, PaletteTheme* theme)
{
    ConfigCursor cursor = {text, length, 0};
    PaletteTheme parsed = TETONOR_DEFAULT_PALETTE_THEME;
    if (length > TETONOR_CONFIG_TEXT_MAX ||
        !expect_text(&cursor, file_header) || !expect_line_end(&cursor) ||
        !expect_text(&cursor, theme_line_key) ||
        !expect_theme_key(&cursor, &parsed))
    {
        return false;
    }
    /* The last line end is optional; nothing may follow. */
    if (!at_end(&cursor) && (!expect_line_end(&cursor) || !at_end(&cursor)))
    {
        return false;
    }
    *theme = parsed;
    return true;
}

ConfigLoadResult load_config_file(const char* path, PaletteTheme* theme)
{
    *theme = TETONOR_DEFAULT_PALETTE_THEME;
    /* One byte more than the maximum tells an oversized file from a full one.
     */
    char text[TETONOR_CONFIG_TEXT_MAX + 1];
    size_t length = 0;
    switch (read_text_file(path, text, sizeof text, &length))
    {
        case TEXT_FILE_READ_OK:
            break;
        case TEXT_FILE_READ_MISSING:
            return CONFIG_LOAD_MISSING;
        case TEXT_FILE_READ_FAILED:
            return CONFIG_LOAD_CORRUPTED;
    }
    return parse_config(text, length, theme) ? CONFIG_LOAD_LOADED
                                             : CONFIG_LOAD_CORRUPTED;
}

bool save_config_file(const char* path, int theme)
{
    char text[TETONOR_CONFIG_TEXT_MAX + 1];
    return serialize_config(theme, text, sizeof text) &&
           write_text_file_atomic(path, text);
}

ConfigLoadResult init_config_store(ConfigStore* store, const char* path)
{
    store->theme      = TETONOR_DEFAULT_PALETTE_THEME;
    store->persistent = false;
    store->path[0]    = '\0';
    size_t length     = path == NULL ? 0 : strlen(path);
    if (length == 0 || length >= TETONOR_CONFIG_PATH_MAX)
    {
        return CONFIG_LOAD_DISABLED;
    }
    memcpy(store->path, path, length + 1);
    store->persistent = true;
    return load_config_file(store->path, &store->theme);
}

bool set_config_store_theme(ConfigStore* store, int theme)
{
    if (!is_palette_theme(theme))
    {
        return false;
    }
    store->theme = (PaletteTheme)theme;
    return !store->persistent || save_config_file(store->path, theme);
}
