#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif

#include <errno.h>
#include <string.h>
#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "user_data_dir.h"

#define TETONOR_STATS_FILE_NAME "statistics.txt"
#define USER_DATA_DIR_MODE 0700

typedef struct
{
    const char* env_name;
    char separator;
    const char* const* segments; /* created in order below the base */
    size_t segment_count;
} PlatformLayout;

static const char* const windows_segments[] = {"Tetonor"};
static const char* const macos_segments[]   = {"Library", "Application Support",
                                               "Tetonor"};
static const char* const linux_segments[]   = {".local", "share", "tetonor"};

#define SEGMENT_COUNT(segments) (sizeof(segments) / sizeof((segments)[0]))

#define WINDOWS_SEPARATOR '\\'
#define POSIX_SEPARATOR '/'

static const PlatformLayout windows_layout = {"LOCALAPPDATA", WINDOWS_SEPARATOR,
                                              windows_segments,
                                              SEGMENT_COUNT(windows_segments)};
static const PlatformLayout macos_layout   = {
    "HOME", POSIX_SEPARATOR, macos_segments, SEGMENT_COUNT(macos_segments)};
static const PlatformLayout linux_layout = {
    "HOME", POSIX_SEPARATOR, linux_segments, SEGMENT_COUNT(linux_segments)};

UserDataPlatform current_user_data_platform(void)
{
#if defined(_WIN32)
    return USER_DATA_PLATFORM_WINDOWS;
#elif defined(__APPLE__)
    return USER_DATA_PLATFORM_MACOS;
#else
    return USER_DATA_PLATFORM_LINUX;
#endif
}

static const PlatformLayout* platform_layout(UserDataPlatform platform)
{
    switch (platform)
    {
        case USER_DATA_PLATFORM_WINDOWS:
            return &windows_layout;
        case USER_DATA_PLATFORM_MACOS:
            return &macos_layout;
        case USER_DATA_PLATFORM_LINUX:
        default:
            return &linux_layout;
    }
}

const char* user_data_env_name(UserDataPlatform platform)
{
    return platform_layout(platform)->env_name;
}

/* The only OS-specific call besides the platform choice above. */
static bool make_directory(const char* path)
{
#if defined(_WIN32)
    int status = _mkdir(path);
#else
    int status = mkdir(path, USER_DATA_DIR_MODE);
#endif
    return status == 0 || errno == EEXIST;
}

/* Appends text to out when it fits together with the terminator. */
static bool append_text(char* out, size_t size, size_t* used, const char* text)
{
    size_t length = strlen(text);
    if (length >= size - *used)
    {
        return false;
    }
    memcpy(out + *used, text, length + 1);
    *used += length;
    return true;
}

static bool append_separator(char* out, size_t size, size_t* used, char sep)
{
    char text[2] = {sep, '\0'};
    return append_text(out, size, used, text);
}

static bool ends_with_separator(const char* out, size_t used)
{
    return out[used - 1] == POSIX_SEPARATOR ||
           out[used - 1] == WINDOWS_SEPARATOR;
}

/* Appends one directory segment, creating the directory when asked. */
static bool append_segment(const PlatformLayout* layout, const char* segment,
                           bool create, char* out, size_t size, size_t* used)
{
    if (!ends_with_separator(out, *used) &&
        !append_separator(out, size, used, layout->separator))
    {
        return false;
    }
    return append_text(out, size, used, segment) &&
           (!create || make_directory(out));
}

static bool build_path(UserDataPlatform platform, const char* base_dir,
                       bool create, char* out, size_t size)
{
    const PlatformLayout* layout = platform_layout(platform);
    size_t used                  = 0;
    if (!append_text(out, size, &used, base_dir))
    {
        return false;
    }
    for (size_t i = 0; i < layout->segment_count; i++)
    {
        if (!append_segment(layout, layout->segments[i], create, out, size,
                            &used))
        {
            return false;
        }
    }
    return append_segment(layout, TETONOR_STATS_FILE_NAME, false, out, size,
                          &used);
}

static bool build_or_empty(UserDataPlatform platform, const char* base_dir,
                           bool create, char* out, size_t size)
{
    if (out == NULL || size == 0)
    {
        return false;
    }
    out[0] = '\0';
    if (base_dir == NULL || base_dir[0] == '\0' ||
        !build_path(platform, base_dir, create, out, size))
    {
        out[0] = '\0';
        return false;
    }
    return true;
}

bool build_stats_file_path(UserDataPlatform platform, const char* base_dir,
                           char* out, size_t size)
{
    return build_or_empty(platform, base_dir, false, out, size);
}

bool prepare_stats_file_path(UserDataPlatform platform, const char* base_dir,
                             char* out, size_t size)
{
    return build_or_empty(platform, base_dir, true, out, size);
}
