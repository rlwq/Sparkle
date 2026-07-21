#include "string_view.h"
#include "ctype.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define issign_sv(c_) ((c_) == '+' || (c_) == '-')

// Wrapped in do/while so it is a single statement: a bare `if` here would let a
// trailing `else` at the call site bind to the macro's `if`.
#define TRUNC(x_, y_)                                                                              \
    do {                                                                                           \
        if ((x_) > (y_))                                                                           \
            (x_) = (y_);                                                                           \
    } while (0)

StringView sv_drop(StringView sv, size_t size) {
    TRUNC(size, sv.size);
    sv.size -= size;
    sv.data += size;
    return sv;
}

StringView sv_take(StringView sv, size_t size) {
    TRUNC(size, sv.size);
    sv.size = size;
    return sv;
}

StringView sv_drop_end(StringView sv, size_t size) {
    TRUNC(size, sv.size);
    sv.size -= size;
    return sv;
}

StringView sv_take_end(StringView sv, size_t size) {
    TRUNC(size, sv.size);
    sv.data += sv.size - size;
    sv.size = size;
    return sv;
}

StringView sv_shrink(StringView sv, size_t size) {
    TRUNC(size, sv.size);

    if (2 * size >= sv.size) {
        sv.size = 0;
        return sv;
    }

    sv.size -= 2 * size;
    sv.data += size;
    return sv;
}

size_t sv_prefix_size(StringView sv, char c) {
    size_t size = 0;
    while (size < sv.size && sv.data[size] == c)
        size++;
    return size;
}

char sv_head(StringView sv) {
    if (sv.size == 0)
        return '\0';
    return sv.data[0];
}

char sv_next(StringView sv) {
    if (sv.size < 2)
        return '\0';
    return sv.data[1];
}

size_t sv_find(StringView sv, char c) {
    char *end = memchr(sv.data, c, sv.size);
    return end ? (size_t)(end - sv.data) : sv.size;
}

StringView sv_drop_ws(StringView sv) {
    size_t size = 0;
    while (size < sv.size && isspace(sv_at(sv, size)))
        size++;
    return sv_drop(sv, size);
}

static int digit_value(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return c - 'A' + 10;
}

// Expects a view sv_scan_number classified as SV_NUMBER_INTEGER, including a
// 0b/0o/0x prefix.
long long int svtolli(StringView sv) {
    assert(sv.size > 0);

    long long int result = 0;
    int sign = 1;
    int base = 10;

    if (sv_head(sv) == '+')
        sv = sv_drop(sv, 1);
    else if (sv_head(sv) == '-') {
        sv = sv_drop(sv, 1);
        sign = -1;
    }

    if (sv.size > 2 && sv_head(sv) == '0') {
        switch (sv_next(sv)) {
        case 'b':
            base = 2;
            break;
        case 'o':
            base = 8;
            break;
        case 'x':
            base = 16;
            break;
        default:
            break;
        }
        if (base != 10)
            sv = sv_drop(sv, 2);
    }

    for (size_t i = 0; i < sv.size; i++)
        result = base * result + digit_value(sv_at(sv, i));

    return sign * result;
}

// Counts the digits at offset i, leaving i past them.
static size_t take_digits(StringView sv, size_t *i) {
    size_t start = *i;
    while (*i < sv.size && isdigit((unsigned char)sv_at(sv, *i)))
        (*i)++;
    return *i - start;
}

static bool is_binary(char c) {
    return c == '0' || c == '1';
}

static bool is_octal(char c) {
    return c >= '0' && c <= '7';
}

static bool is_hex(char c) {
    return isdigit((unsigned char)c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

// Returns the digit test for a 0b/0o/0x prefix at offset i, or NULL when there
// is none.
static bool (*radix_digits(StringView sv, size_t i))(char) {
    if (i + 1 >= sv.size || sv_at(sv, i) != '0')
        return NULL;

    switch (sv_at(sv, i + 1)) {
    case 'b':
        return is_binary;
    case 'o':
        return is_octal;
    case 'x':
        return is_hex;
    default:
        return NULL;
    }
}

SvNumber sv_scan_number(StringView sv, size_t *length) {
    size_t i = 0;

    if (i < sv.size && issign_sv(sv_at(sv, i)))
        i++;

    // A radix literal is decided before the decimal path, since 0b1010 starts
    // with a digit the decimal scan would happily take on its own. A prefix
    // with no digits after it is not one: 0b reads as 0 and the symbol b.
    bool (*is_radix_digit)(char) = radix_digits(sv, i);
    if (is_radix_digit) {
        size_t after = i + 2;
        size_t start = after;
        while (after < sv.size && is_radix_digit(sv_at(sv, after)))
            after++;

        if (after > start) {
            *length = after;
            return SV_NUMBER_INTEGER;
        }
    }

    size_t whole = take_digits(sv, &i);
    bool is_float = false;

    // A point belongs to the number only if a digit sits on one side of it, so
    // a lone '.' stays the symbol it has always been.
    if (i < sv.size && sv_at(sv, i) == '.') {
        size_t after = i + 1;
        size_t fraction = take_digits(sv, &after);
        if (whole > 0 || fraction > 0) {
            is_float = true;
            i = after;
        }
    }

    if (whole == 0 && !is_float)
        return SV_NUMBER_NONE;

    // An exponent with no digits is not part of the number: "1e" reads as the
    // integer 1 followed by the symbol e, rather than failing to lex.
    if (i < sv.size && (sv_at(sv, i) == 'e' || sv_at(sv, i) == 'E')) {
        size_t after = i + 1;
        if (after < sv.size && issign_sv(sv_at(sv, after)))
            after++;
        if (take_digits(sv, &after) > 0) {
            is_float = true;
            i = after;
        }
    }

    *length = i;
    return is_float ? SV_NUMBER_FLOAT : SV_NUMBER_INTEGER;
}

// The view is not NUL-terminated, so it is copied before strtod sees it. The
// program never calls setlocale, so the decimal point stays '.' as the grammar
// expects. sv_scan_number has already established the shape; strtod is here to
// round correctly, which hand-rolled exponent arithmetic does not.
double svtod(StringView sv) {
    char inline_buffer[64];
    char *buffer = inline_buffer;

    if (sv.size + 1 > sizeof(inline_buffer)) {
        buffer = malloc(sv.size + 1);
        assert(buffer);
    }

    memcpy(buffer, sv.data, sv.size);
    buffer[sv.size] = '\0';

    double result = strtod(buffer, NULL);

    if (buffer != inline_buffer)
        free(buffer);

    return result;
}
