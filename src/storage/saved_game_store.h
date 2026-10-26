#ifndef TETONOR_STORAGE_SAVED_GAME_STORE_H
#define TETONOR_STORAGE_SAVED_GAME_STORE_H

#include <stdbool.h>

#include "text_file.h"
#include "../game/saved_game.h"

/* longest saved game file path, with '\0' */
#define TETONOR_SAVED_GAME_PATH_MAX TETONOR_TEXT_FILE_PATH_MAX

typedef enum
{
    SAVED_GAME_LOAD_LOADED,
    SAVED_GAME_LOAD_MISSING,   /* no readable file: no saved game */
    SAVED_GAME_LOAD_CORRUPTED, /* invalid content: no saved game */
    SAVED_GAME_LOAD_DISABLED   /* no path: memory only for this session */
} SavedGameLoadResult;

typedef enum
{
    SAVED_GAME_SAVE_STORED,       /* kept in memory and written to the file */
    SAVED_GAME_SAVE_SESSION_ONLY, /* kept in memory; no file, or write failed */
    SAVED_GAME_SAVE_REJECTED      /* invalid game; nothing changed */
} SavedGameSaveResult;

/* The single saved game and where it persists. A fixed-size value owned by
 * the composition root: nothing to release. */
typedef struct
{
    SavedGame game; /* meaningful only when available */
    bool available;
    bool persistent;                        /* false: memory only */
    char path[TETONOR_SAVED_GAME_PATH_MAX]; /* meaningful only when persistent
                                             */
} SavedGameStore;

/* Loads path into store. NULL, empty or too long gives memory only. Always
 * leaves store usable; never alters the file. */
SavedGameLoadResult init_saved_game_store(SavedGameStore* store,
                                          const char* path);

/* The saved game, or NULL when there is none. */
const SavedGame* saved_game_store_game(const SavedGameStore* store);

/* Validates, keeps the game in memory, then writes it when persistent. A failed
 * write keeps the previous file. Never blocks or aborts. */
SavedGameSaveResult save_saved_game_store(SavedGameStore* store,
                                          const SavedGame* game);

/* Forgets the saved game and removes the file when persistent; a failed
 * removal is ignored. */
void discard_saved_game_store(SavedGameStore* store);

/* File primitives, public for tests. load_saved_game_file never returns
 * SAVED_GAME_LOAD_DISABLED and leaves *game untouched unless it returns
 * SAVED_GAME_LOAD_LOADED. save_saved_game_file never leaves a partial file at
 * path. */
SavedGameLoadResult load_saved_game_file(const char* path, SavedGame* game);
bool save_saved_game_file(const char* path, const SavedGame* game);

#endif
