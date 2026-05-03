#include "scope.h"
#include "dynamic_array.h"
#include "string_view.h"
#include <assert.h>

bool scope_define(Scope *scope, StringView name, LispNode *value) {
    bool is_defined = false;
    for (size_t i = 0; i < scope->items.size; i++) {
        if (sv_eq(da_at(scope->items, i).key, name)) {
            is_defined = true;
            break;
        }
    }
    if (is_defined)
        return false;
    ScopeItem item = {.key = name, .value = value};
    da_push(scope->items, item);
    return true;
}

LispNode *scope_get(Scope *scope, StringView name) {
    for (Scope *curr = scope; curr != NULL; curr = curr->parent) {
        for (size_t i = 0; i < curr->items.size; i++) {
            ScopeItem item = da_at(curr->items, i);
            if (sv_eq(item.key, name))
                return item.value;
        }
    }
    return NULL;
}
