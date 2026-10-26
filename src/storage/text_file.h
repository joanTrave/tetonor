#ifndef TETONOR_STORAGE_TEXT_FILE_H
#define TETONOR_STORAGE_TEXT_FILE_H

#include <stdbool.h>
#include <stddef.h>

/* Longest data file path, with the terminator. */
#define TETONOR_TEXT_FILE_PATH_MAX 1024

typedef enum
{
    TEXT_FILE_READ_OK,
    TEXT_FILE_READ_MISSING, /* the file cannot be opened */
    TEXT_FILE_READ_FAILED   /* read error, or longer than max_length */
} TextFileReadResult;

/* Reads the whole file into text. text must hold max_length + 1 bytes; *length
 * is set only on TEXT_FILE_READ_OK. */
TextFileReadResult read_text_file(const char* path, char* text,
                                  size_t max_length, size_t* length);

/* Writes text to path + ".tmp", then replaces path. Never leaves a partial
 * file at path and removes the temporary on failure. Returns false when the
 * path is too long or any step fails. */
bool write_text_file_atomically(const char* path, const char* text);

#endif
