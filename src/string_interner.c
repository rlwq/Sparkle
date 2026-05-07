#include "string_interner.h"
#include "dynamic_array.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


StringInterner *si_alloc(void) {
    StringInterner *si = malloc(sizeof(StringInterner));
    assert(si);
    
    da_init(si->strings);

    return si;
}

void si_free(StringInterner *si) {
    for (size_t i = 0; i < si->strings.size; i++)
        free(da_at(si->strings, i));

    da_free(si->strings);
    free(si);
}

bool comp_nstr_cstr(const char *nstr, size_t size, const char *cstr) {
    for (size_t i = 0; i < size; i++)
        if (cstr[i] == '\0' || nstr[i] != cstr[i]) return false;

    return cstr[size] == '\0';
}

StringName si_getn(StringInterner *si, const char *str, size_t n) {
    for (size_t i = 0; i < si->strings.size; i++)
        if (comp_nstr_cstr(str, n, da_at(si->strings, i)))
            return da_at(si->strings, i);
    
    char *sn = malloc(n + 1);
    strncpy(sn, str, n);
    sn[n] = '\0';
    da_push(si->strings, sn);
    return sn;
}

StringName si_get(StringInterner *si, const char *str) {
    return si_getn(si, str, strlen(str));
}

