#ifndef TETONOR_STORAGE_STATS_STORE_H
#define TETONOR_STORAGE_STATS_STORE_H

#include <stdbool.h>
#include <stdint.h>

#include "../game/statistics.h"
#include "text_file.h"

#define TETONOR_STATS_PATH_MAX TETONOR_TEXT_FILE_PATH_MAX

typedef enum
{
    STATS_LOAD_LOADED,
    STATS_LOAD_MISSING,   /* no readable file: empty statistics */
    STATS_LOAD_CORRUPTED, /* unreadable content: empty statistics */
    STATS_LOAD_DISABLED   /* no path: memory only for this session */
} StatsLoadResult;

/* The session's statistics and where they persist. A fixed-size value owned
 * by the composition root: nothing to release. */
typedef struct
{
    Statistics statistics;
    bool persistent;                   /* false: memory only */
    char path[TETONOR_STATS_PATH_MAX]; /* meaningful only when persistent */
} StatsStore;

/* Loads path into store. NULL, empty or too long gives memory only. Always
 * leaves store usable. */
StatsLoadResult init_stats_store(StatsStore* store, const char* path);

/* Records one completed game in memory, then saves when persistent. Returns
 * true when recorded and (if persistent) saved; false otherwise. Never blocks
 * or aborts. */
bool record_stats_store_game(StatsStore* store, int difficulty,
                             int32_t seconds);

/* File primitives, public for tests. load_statistics_file never returns
 * STATS_LOAD_DISABLED and leaves empty statistics unless it returns
 * STATS_LOAD_LOADED. save_statistics_file never leaves a partial file at
 * path. */
StatsLoadResult load_statistics_file(const char* path, Statistics* statistics);
bool save_statistics_file(const char* path, const Statistics* statistics);

#endif
