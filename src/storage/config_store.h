#ifndef TETONOR_STORAGE_CONFIG_STORE_H
#define TETONOR_STORAGE_CONFIG_STORE_H

#include <stdbool.h>
#include <stddef.h>

#include "text_file.h"
#include "../game/palette.h"

#define TETONOR_CONFIG_FILE_NAME "config.txt"
#define TETONOR_CONFIG_PATH_MAX TETONOR_TEXT_FILE_PATH_MAX
/* Longest config file accepted, in bytes. */
#define TETONOR_CONFIG_TEXT_MAX 64

typedef enum
{
    CONFIG_LOAD_LOADED,
    CONFIG_LOAD_MISSING,   /* no readable file: default theme */
    CONFIG_LOAD_CORRUPTED, /* unreadable or unknown content: default theme */
    CONFIG_LOAD_DISABLED   /* no path: memory only for this session */
} ConfigLoadResult;

/* The player's settings and where they persist. A fixed-size value owned by
 * the composition root: nothing to release. */
typedef struct
{
    PaletteTheme theme;
    bool persistent;                    /* false: memory only */
    char path[TETONOR_CONFIG_PATH_MAX]; /* meaningful only when persistent */
} ConfigStore;

/* Loads path into store. NULL, empty or too long gives memory only. Always
 * leaves store usable, with the default theme unless the file holds another.
 */
ConfigLoadResult init_config_store(ConfigStore* store, const char* path);

/* Sets the theme in memory, then saves when persistent. Returns true when set
 * and (if persistent) saved; false for a value that is not a theme (nothing
 * changes) or a failed save (the new theme stays in memory). Never aborts. */
bool set_config_store_theme(ConfigStore* store, int theme);

/* Writes "TETONOR-CONFIG 1\ntheme <key>\n" into out. False when it does not
 * fit with its terminator or theme is not a theme. */
bool serialize_config(int theme, char* out, size_t size);

/* Accepts exactly the serialized form, with "\n" or "\r\n" line ends and an
 * optional last one. False, leaving *theme untouched, on anything else or on
 * more than TETONOR_CONFIG_TEXT_MAX bytes. */
bool parse_config(const char* text, size_t length, PaletteTheme* theme);

/* Sets *theme to the default, then to the file's theme on LOADED. MISSING when
 * the file cannot be opened, CORRUPTED when it is unreadable, oversized, empty
 * or unparsable. Never DISABLED. */
ConfigLoadResult load_config_file(const char* path, PaletteTheme* theme);

/* Writes the config through an atomic replace: no partial file is left at
 * path. False on any failure. */
bool save_config_file(const char* path, int theme);

#endif
