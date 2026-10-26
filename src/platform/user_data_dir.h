#ifndef TETONOR_PLATFORM_USER_DATA_DIR_H
#define TETONOR_PLATFORM_USER_DATA_DIR_H

#include <stdbool.h>
#include <stddef.h>

typedef enum
{
    USER_DATA_PLATFORM_LINUX, /* also any other non-Windows, non-Apple OS */
    USER_DATA_PLATFORM_MACOS,
    USER_DATA_PLATFORM_WINDOWS
} UserDataPlatform;

/* The platform this binary was built for. */
UserDataPlatform current_user_data_platform(void);

/* "LOCALAPPDATA" on Windows, "HOME" elsewhere. */
const char* user_data_env_name(UserDataPlatform platform);

/* base_dir + the platform's relative directory + "statistics.txt". Pure.
 * Returns false (out emptied) on a NULL or empty base or when out is too
 * small. */
bool build_stats_file_path(UserDataPlatform platform, const char* base_dir,
                           char* out, size_t size);

/* build_stats_file_path, then creates every missing directory below base_dir
 * (never base_dir itself). Returns false when the path cannot be built or a
 * directory cannot be created. */
bool prepare_stats_file_path(UserDataPlatform platform, const char* base_dir,
                             char* out, size_t size);

/* As build_stats_file_path, ending in "saved_game.txt". */
bool build_saved_game_file_path(UserDataPlatform platform, const char* base_dir,
                                char* out, size_t size);

/* As prepare_stats_file_path, ending in "saved_game.txt". */
bool prepare_saved_game_file_path(UserDataPlatform platform,
                                  const char* base_dir, char* out, size_t size);

#endif
