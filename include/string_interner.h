#ifndef STRING_INTERNER_H
#define STRING_INTERNER_H

#include "dynamic_array.h"
#include <stddef.h>

typedef const char* StringName;

typedef struct {
    DA(char*) strings;
} StringInterner;


StringInterner *si_alloc(void);
void si_free(StringInterner *si);

StringName si_getn(StringInterner *si, const char *str, size_t n);
StringName si_get(StringInterner *si, const char *str);

#endif
