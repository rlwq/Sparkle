#ifndef BUILTINS_H
#define BUILTINS_H

#include "vm.h"

typedef struct {
    const char *name;
    LispBuiltin func;
} BuiltinDef;

extern const BuiltinDef BUILTINS[];
extern const size_t BUILTINS_COUNT;

#endif
