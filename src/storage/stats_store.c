#include <stdio.h>
#include <string.h>

#include "stats_store.h"

#define STATS_TMP_SUFFIX ".tmp"

StatsLoadResult load_statistics_file(const char* path, Statistics* statistics)
{
    init_statistics(statistics);
    FILE* file = fopen(path, "rb");
    if (file == NULL)
    {
        return STATS_LOAD_MISSING;
    }
    /* One byte more than the maximum tells an oversized file from a full one.
     */
    char text[TETONOR_STATS_TEXT_MAX + 1];
    size_t length = fread(text, 1, sizeof text, file);
    bool failed   = ferror(file) != 0;
    fclose(file);
    if (failed || length > TETONOR_STATS_TEXT_MAX ||
        !parse_statistics(text, length, statistics))
    {
        return STATS_LOAD_CORRUPTED;
    }
    return STATS_LOAD_LOADED;
}

static bool write_text_file(const char* path, const char* text)
{
    FILE* file = fopen(path, "wb");
    if (file == NULL)
    {
        return false;
    }
    size_t length  = strlen(text);
    size_t written = fwrite(text, 1, length, file);
    return fclose(file) == 0 && written == length;
}

/* rename() replaces an existing file on POSIX but fails on Windows, so a
 * failed first attempt removes the old file and tries once more. */
static bool replace_file(const char* temporary, const char* path)
{
    return rename(temporary, path) == 0 ||
           (remove(path) == 0 && rename(temporary, path) == 0);
}

bool save_statistics_file(const char* path, const Statistics* statistics)
{
    if (strlen(path) >= TETONOR_STATS_PATH_MAX)
    {
        return false;
    }
    char text[TETONOR_STATS_TEXT_MAX + 1];
    char temporary[TETONOR_STATS_PATH_MAX + sizeof STATS_TMP_SUFFIX];
    snprintf(temporary, sizeof temporary, "%s" STATS_TMP_SUFFIX, path);
    if (!serialize_statistics(statistics, text, sizeof text))
    {
        return false;
    }
    if (!write_text_file(temporary, text) || !replace_file(temporary, path))
    {
        remove(temporary);
        return false;
    }
    return true;
}

StatsLoadResult init_stats_store(StatsStore* store, const char* path)
{
    init_statistics(&store->statistics);
    store->persistent = false;
    store->path[0]    = '\0';
    size_t length     = path == NULL ? 0 : strlen(path);
    if (length == 0 || length >= TETONOR_STATS_PATH_MAX)
    {
        return STATS_LOAD_DISABLED;
    }
    memcpy(store->path, path, length + 1);
    store->persistent = true;
    return load_statistics_file(store->path, &store->statistics);
}

bool record_stats_store_game(StatsStore* store, int difficulty, int32_t seconds)
{
    if (!record_completed_game(&store->statistics, difficulty, seconds))
    {
        return false;
    }
    return !store->persistent ||
           save_statistics_file(store->path, &store->statistics);
}
