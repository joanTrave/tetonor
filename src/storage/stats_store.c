#include <string.h>

#include "stats_store.h"

StatsLoadResult load_statistics_file(const char* path, Statistics* statistics)
{
    init_statistics(statistics);
    char text[TETONOR_STATS_TEXT_MAX + 1];
    size_t length = 0;
    switch (read_text_file(path, text, TETONOR_STATS_TEXT_MAX, &length))
    {
        case TEXT_FILE_READ_OK:
            break;
        case TEXT_FILE_READ_MISSING:
            return STATS_LOAD_MISSING;
        case TEXT_FILE_READ_FAILED:
            return STATS_LOAD_CORRUPTED;
    }
    return parse_statistics(text, length, statistics) ? STATS_LOAD_LOADED
                                                      : STATS_LOAD_CORRUPTED;
}

bool save_statistics_file(const char* path, const Statistics* statistics)
{
    char text[TETONOR_STATS_TEXT_MAX + 1];
    if (!serialize_statistics(statistics, text, sizeof text))
    {
        return false;
    }
    return write_text_file_atomically(path, text);
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
