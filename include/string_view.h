#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include "dynamic_array.h"

/* String View Data Structure */

typedef struct {
    const char *data;
    size_t size;
} StringView;

typedef DA(StringView) SV_DA;

#define SV_FMT "%.*s"
#define SV_ARGS(sv_) ((int) (sv_).size), (sv_).data

#define sv(s_, n_) ( (StringView) { .data = (s_), .size = (n_) } )
#define sv_mk(s_) ( (StringView) { .data = (s_), .size = strlen(s_) } )
#define sv_at(sv_, i_) ( (sv_).data[i_] )
#define sv_size(sv_) ( (sv_).size )
#define sv_eq(sv1_, sv2_) ( (sv1_).size == (sv2_).size && !memcmp((sv1_).data, (sv2_).data, (sv1_).size) )

#define sv_is_empty(sv_) ((sv_).size == 0)

// TODO: review this macro
#define sv_dup(sv_) ( (StringView) { .data = strndup((sv_).data, (sv_).size), .size = (sv_).size } )

StringView sv_drop(StringView sv, size_t size);
StringView sv_take(StringView sv, size_t size);
StringView sv_drop_end(StringView sv, size_t size);
StringView sv_take_end(StringView sv, size_t size);
StringView sv_shrink(StringView sv, size_t size);

size_t sv_prefix_size(StringView sv, char c);

char sv_head(StringView sv);
char sv_next(StringView sv);
bool sv_peek(StringView sv, char c);
bool sv_eat(StringView *sv, char c);

size_t sv_find(StringView sv, char c);

StringView sv_drop_ws(StringView sv);

int svtoi(StringView sv);

/* String Builder Data Structure */

typedef struct {
    char *data;
    size_t size;
} StringBuilder;

// TODO: add custom allocation
#define sb(n_) ( (StringBuilder) { .data = malloc(n_), .size = 0 } )
#define sb_free(sb_) do {  \
        free((sb_).data);  \
        (sb_).size = 0;    \
    } while (0)

#endif
