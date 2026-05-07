#ifndef SCOPE_H
#define SCOPE_H

#include "forwards.h"
#include "stdbool.h"
#include "string_interner.h"

typedef struct {
    StringName key;
    LispNode *value;
} ScopeItem;

struct Scope {
    Scope *parent;

    DA(ScopeItem) items;

    Scope *heap_next;
    bool marked;
};

bool scope_define(Scope *scope, StringName name, LispNode *value);
LispNode *scope_get(Scope *scope, StringName name);

#endif
