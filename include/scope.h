#ifndef SCOPE_H
#define SCOPE_H

#include "forwards.h"
#include "stdbool.h"
#include "string_interner.h"

typedef struct {
    StringName key;
    Object *value;
} ScopeItem;

struct Scope {
    Scope *parent;

    DA(ScopeItem) items;

    Scope *heap_next;
    bool marked;
};

bool scope_define(Scope *scope, StringName name, Object *value);
Object *scope_get(Scope *scope, StringName name);
bool scope_set(Scope *scope, StringName name, Object *value);

#endif
