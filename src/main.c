#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "platform/user_data_dir.h"
#include "storage/saved_game_store.h"
#include "storage/stats_store.h"
#include "ui/menu.h"
#include "ui/screen.h"

static void init_random_generator(void)
{
    srand((unsigned)time(NULL));
}

/* Loads the statistics from the per-user data directory. When that directory
 * is unavailable the game still runs, keeping its statistics in memory. */
static void open_statistics(StatsStore* stats_store)
{
    UserDataPlatform platform = current_user_data_platform();
    const char* base_dir      = getenv(user_data_env_name(platform));
    char path[TETONOR_STATS_PATH_MAX];
    bool ready = prepare_stats_file_path(platform, base_dir, path, sizeof path);
    init_stats_store(stats_store, ready ? path : NULL);
}

/* Loads the saved game from the same directory. When that directory is
 * unavailable, or the file is missing or damaged, the game starts without a
 * saved game and keeps any new one in memory. */
static void open_saved_game(SavedGameStore* saved_game_store)
{
    UserDataPlatform platform = current_user_data_platform();
    const char* base_dir      = getenv(user_data_env_name(platform));
    char path[TETONOR_SAVED_GAME_PATH_MAX];
    bool ready =
        prepare_saved_game_file_path(platform, base_dir, path, sizeof path);
    init_saved_game_store(saved_game_store, ready ? path : NULL);
}

int main(void)
{
    init_random_generator();
    StatsStore stats_store;
    open_statistics(&stats_store);
    SavedGameStore saved_game_store;
    open_saved_game(&saved_game_store);
    init_screen();
    int status = ui_show_start_menu(&stats_store, &saved_game_store);
    end_screen();
    if (status != EXIT_SUCCESS)
    {
        fprintf(stderr, "tetonor: failed to start the game\n");
    }
    return status;
}
