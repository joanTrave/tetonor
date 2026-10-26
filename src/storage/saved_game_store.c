#include <stdio.h>
#include <string.h>

#include "saved_game_store.h"

SavedGameLoadResult load_saved_game_file(const char* path, SavedGame* game)
{
    /* One byte more than the maximum tells an oversized file from a full one.
     */
    char text[TETONOR_SAVED_GAME_TEXT_MAX + 1];
    size_t length = 0;
    switch (read_text_file(path, text, sizeof text, &length))
    {
        case TEXT_FILE_READ_OK:
            break;
        case TEXT_FILE_READ_MISSING:
            return SAVED_GAME_LOAD_MISSING;
        case TEXT_FILE_READ_FAILED:
            return SAVED_GAME_LOAD_CORRUPTED;
    }
    return parse_saved_game(text, length, game) ? SAVED_GAME_LOAD_LOADED
                                                : SAVED_GAME_LOAD_CORRUPTED;
}

bool save_saved_game_file(const char* path, const SavedGame* game)
{
    char text[TETONOR_SAVED_GAME_TEXT_MAX + 1];
    if (!serialize_saved_game(game, text, sizeof text))
    {
        return false;
    }
    return write_text_file_atomic(path, text);
}

SavedGameLoadResult init_saved_game_store(SavedGameStore* store,
                                          const char* path)
{
    store->available  = false;
    store->persistent = false;
    store->path[0]    = '\0';
    size_t length     = path == NULL ? 0 : strlen(path);
    if (length == 0 || length >= TETONOR_SAVED_GAME_PATH_MAX)
    {
        return SAVED_GAME_LOAD_DISABLED;
    }
    memcpy(store->path, path, length + 1);
    store->persistent = true;
    SavedGameLoadResult result =
        load_saved_game_file(store->path, &store->game);
    store->available = result == SAVED_GAME_LOAD_LOADED;
    return result;
}

const SavedGame* saved_game_store_game(const SavedGameStore* store)
{
    return store->available ? &store->game : NULL;
}

SavedGameSaveResult save_saved_game_store(SavedGameStore* store,
                                          const SavedGame* game)
{
    if (!is_saved_game_valid(game))
    {
        return SAVED_GAME_SAVE_REJECTED;
    }
    store->game      = *game;
    store->available = true;
    if (store->persistent && save_saved_game_file(store->path, game))
    {
        return SAVED_GAME_SAVE_STORED;
    }
    return SAVED_GAME_SAVE_SESSION_ONLY;
}

void discard_saved_game_store(SavedGameStore* store)
{
    store->available = false;
    if (store->persistent)
    {
        remove(store->path);
    }
}
