#include <string.h>
#include "string_view.h"
#include "ctype.h"

#define TRUNC(x_, y_) if ((x_) > (y_)) (x_) = (y_)

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
    if (sv.size == 0) return '\0';
    return sv.data[0];
}

StringView sv_drop_ws(StringView sv) {
    size_t size = 0;
    while (size < sv.size && isspace(sv_at(sv, size))) size++;
    return sv_drop(sv, size);
}

StringView sv_alloc_replace(StringView sv, char from, char to) {
    char *new_data = strndup(sv.data, sv.size);
    
    for (size_t i = 0; i < sv.size; i++)
        if (sv_at(sv, i) == from)
            new_data[i] = to;
    return (StringView) { .data = new_data, .size = sv.size };
}

// TODO: unsafe, no sound logic
int svtoi(StringView sv) {
    int result = 0;
    
    for (size_t i = 0; i < sv.size; i++)
        result = 10 * result + sv_at(sv, i) - '0';

    return result;
}


