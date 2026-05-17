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
    X(lambda)                                                                                      \
    X(TYPE_EXCEPTION)                                                                              \
    X(ARITY_EXCEPTION)                                                                             \
    X(UNDEFINED_EXCEPTION)                                                                         \
    X(REBINDING_EXCEPTION)                                                                         \
    X(UNCALLABLE_EXCEPTION)                                                                        \
    X(VALUE_EXCEPTION)

typedef const char *StringName;

typedef struct {
    DA(char *) strings;

    struct {
#define X(t_) StringName _##t_;
        PREBUILTS
#undef X
    } prebuilt;
} StringInterner;

StringInterner *si_alloc(void);
void si_free(StringInterner *si);

StringName si_getn(StringInterner *si, const char *str, size_t n);
StringName si_get(StringInterner *si, const char *str);

#endif
