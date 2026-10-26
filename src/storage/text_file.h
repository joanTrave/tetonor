#ifndef TETONOR_STORAGE_TEXT_FILE_H
#define TETONOR_STORAGE_TEXT_FILE_H

#include <stdbool.h>
#include <stddef.h>

/* Longest data file path, with the terminating NUL. */
#define TETONOR_TEXT_FILE_PATH_MAX 1024

typedef enum
{
    TEXT_FILE_READ_OK,
    TEXT_FILE_READ_MISSING, /* cannot be opened */
    TEXT_FILE_READ_FAILED   /* read error, or capacity bytes or more */
} TextFileReadResult;

/* Reads the whole file at path into out (capacity bytes, no terminator added)
 * and its size into *length. */
TextFileReadResult read_text_file(const char* path, char* out, size_t capacity,
                                  size_t* length);

/* Writes text to path + ".tmp", then replaces path with it. Never leaves a
 * partial file at path. False when path is too long or any step fails. */
bool write_text_file_atomic(const char* path, const char* text);

#endif
