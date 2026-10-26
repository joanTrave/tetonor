#include <string.h>

#include "text_format.h"

#define TEXT_DIGIT_BASE 10

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

TextCursor make_text_cursor(const char* text, size_t length)
{
    TextCursor cursor = {text, text + length};
    return cursor;
}

bool read_text_literal(TextCursor* cursor, const char* literal)
{
    size_t length = strlen(literal);
    if ((size_t)(cursor->end - cursor->at) < length ||
        memcmp(cursor->at, literal, length) != 0)
    {
        return false;
    }
    cursor->at += length;
    return true;
}

bool read_text_line_end(TextCursor* cursor)
{
    return read_text_literal(cursor, "\n") || read_text_literal(cursor, "\r\n");
}

bool read_text_number(TextCursor* cursor, int max_digits, int64_t* number)
{
    const char* at = cursor->at;
    int64_t value  = 0;
    int digits     = 0;
    while (at < cursor->end && is_digit(*at))
    {
        if (digits == max_digits)
        {
            return false;
        }
        value = value * TEXT_DIGIT_BASE + (*at - '0');
        digits++;
        at++;
    }
    if (digits == 0)
    {
        return false;
    }
    *number    = value;
    cursor->at = at;
    return true;
}

bool is_text_cursor_at_end(const TextCursor* cursor)
{
    return cursor->at == cursor->end;
}

bool advance_text_output(int written, size_t size, size_t* used)
{
    if (written < 0 || (size_t)written >= size - *used)
    {
        return false;
    }
    *used += (size_t)written;
    return true;
}
