#ifndef SCOPE_H
#define SCOPE_H

#include "forwards.h"
#include "stdbool.h"
#include "string_view.h"

typedef struct {
    StringView key;
    LispNode *value;
} ScopeItem;

struct Scope {
    Scope *parent;

    DA(ScopeItem) items;

    Scope *heap_next;
    bool marked;
};

bool scope_define(Scope *scope, StringView name, LispNode *value);
LispNode *scope_get(Scope *scope, StringView name);

#endif
