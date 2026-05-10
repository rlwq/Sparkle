#ifndef STRING_INTERNER_H
#define STRING_INTERNER_H

#include "dynamic_array.h"
#include <stddef.h>

#define PREBUILTS                                                                                  \
    X(True)                                                                                        \
    X(False)                                                                                       \
    X(Nil)                                                                                         \
    X(while)                                                                                       \
    X(let)                                                                                         \
    X(if)                                                                                          \
    X(set)                                                                                         \
    X(quote)                                                                                       \
    X(lambda)

typedef const char *StringName;

typedef struct {
    DA(char *) strings;

#define X(t_) StringName _##t_;
    struct {
        PREBUILTS
    } prebuilt;
} StringInterner;
#undef X

StringInterner *si_alloc(void);
void si_free(StringInterner *si);

StringName si_getn(StringInterner *si, const char *str, size_t n);
StringName si_get(StringInterner *si, const char *str);

#endif
