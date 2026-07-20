#include "string_view.h"
#include "ctype.h"
#include <assert.h>
#include <string.h>

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

long long int svtolli(StringView sv) {
    assert(sv.size > 0);
    assert(sv_head(sv) == '+' || sv_head(sv) == '-' || isdigit(sv_head(sv)));

    long long int result = 0;
    int sign = 1;

    if (sv_head(sv) == '+')
        sv = sv_drop(sv, 1);
    else if (sv_head(sv) == '-') {
        sv = sv_drop(sv, 1);
        sign = -1;
    }

    for (size_t i = 0; i < sv.size; i++) {
        assert(isdigit(sv_at(sv, i)));
        result = 10 * result + sv_at(sv, i) - '0';
    }

    return sign * result;
}

double svtod(StringView sv) {
    long long int digits = 0;
    int sign = 1;

    double decimals = 0;
    double p = 0.1;

    if (sv_head(sv) == '+')
        sv = sv_drop(sv, 1);
    else if (sv_head(sv) == '-') {
        sv = sv_drop(sv, 1);
        sign = -1;
    }

    size_t i = 0;
    for (; sv_at(sv, i) != '.'; i++)
        digits = 10 * digits + sv_at(sv, i) - '0';

    sv = sv_drop(sv, i + 1);

    for (size_t j = 0; j < sv.size; j++) {
        decimals += p * (sv_at(sv, j) - '0');
        p /= 10;
    }

    return sign * (digits + decimals);
}
