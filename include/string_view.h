#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include "dynamic_array.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* String View Data Structure */

typedef struct {
    const char *data;
    size_t size;
} StringView;

typedef DA(StringView) StringViewDA;

#define SV_FMT "%.*s"
#define SV_ARGS(sv_) ((int)(sv_).size), (sv_).data

#define sv(s_, n_) ((StringView){.data = (s_), .size = (n_)})
#define sv_mk(s_) ((StringView){.data = (s_), .size = strlen(s_)})
#define sv_at(sv_, i_) ((sv_).data[i_])
#define sv_size(sv_) ((sv_).size)
#define sv_eq(sv1_, sv2_)                                                                          \
    ((sv1_).size == (sv2_).size && !memcmp((sv1_).data, (sv2_).data, (sv1_).size))

#define sv_is_empty(sv_) ((sv_).size == 0)

StringView sv_drop(StringView sv, size_t size);
StringView sv_take(StringView sv, size_t size);
StringView sv_drop_end(StringView sv, size_t size);
StringView sv_take_end(StringView sv, size_t size);
StringView sv_shrink(StringView sv, size_t size);

size_t sv_prefix_size(StringView sv, char c);

char sv_head(StringView sv);
char sv_next(StringView sv);

size_t sv_find(StringView sv, char c);

StringView sv_drop_ws(StringView sv);

int svtoi(StringView sv);

#endif
