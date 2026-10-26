#ifndef TETONOR_TEXT_FORMAT_H
#define TETONOR_TEXT_FORMAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* A read position over text that is not NUL-terminated: [at, end). */
typedef struct
{
    const char* at;
    const char* end;
} TextCursor;

TextCursor make_text_cursor(const char* text, size_t length);

/* Each read_* function advances the cursor only on success. */

/* Consumes literal when the text continues with exactly it. */
bool read_text_literal(TextCursor* cursor, const char* literal);

/* Consumes "\n" or "\r\n"; a lone '\r' is not a line end. */
bool read_text_line_end(TextCursor* cursor);

/* Consumes 1..max_digits ASCII digits (no sign, no spaces; leading zeros
 * allowed) and stores their value. max_digits must be at most 18, so the value
 * always fits an int64_t. */
bool read_text_number(TextCursor* cursor, int max_digits, int64_t* number);

bool is_text_cursor_at_end(const TextCursor* cursor);

/* snprintf bookkeeping: adds written to *used. Returns false, leaving *used
 * unchanged, when written is negative or the text did not fit size together
 * with its terminator. */
bool advance_text_output(int written, size_t size, size_t* used);

#endif
