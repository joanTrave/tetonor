#include <stdio.h>
#include <string.h>

#include "text_file.h"

#define TEXT_FILE_TMP_SUFFIX ".tmp"

TextFileReadResult read_text_file(const char* path, char* out, size_t capacity,
                                  size_t* length)
{
    *length    = 0;
    FILE* file = fopen(path, "rb");
    if (file == NULL)
    {
        return TEXT_FILE_READ_MISSING;
    }
    *length     = fread(out, 1, capacity, file);
    bool failed = ferror(file) != 0;
    fclose(file);
    if (failed || *length >= capacity)
    {
        return TEXT_FILE_READ_FAILED;
    }
    return TEXT_FILE_READ_OK;
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

bool write_text_file_atomic(const char* path, const char* text)
{
    if (strlen(path) >= TETONOR_TEXT_FILE_PATH_MAX)
    {
        return false;
    }
    char temporary[TETONOR_TEXT_FILE_PATH_MAX + sizeof TEXT_FILE_TMP_SUFFIX];
    snprintf(temporary, sizeof temporary, "%s" TEXT_FILE_TMP_SUFFIX, path);
    if (!write_text_file(temporary, text) || !replace_file(temporary, path))
    {
        remove(temporary);
        return false;
    }
    return true;
}
