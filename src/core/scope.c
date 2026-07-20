#include "scope.h"
#include "dynamic_array.h"
#include "string_interner.h"
#include <assert.h>
#include <stdbool.h>

bool scope_define(Scope *scope, StringName name, Object *value) {
    bool is_defined = false;
    for (size_t i = 0; i < scope->items.size; i++) {
        if (da_at(scope->items, i).key == name) {
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

Object *scope_get(Scope *scope, StringName name) {
    for (Scope *curr = scope; curr != NULL; curr = curr->parent) {
        for (size_t i = 0; i < curr->items.size; i++) {
            ScopeItem item = da_at(curr->items, i);
            if (item.key == name)
                return item.value;
        }
    }
    return NULL;
}

bool scope_set(Scope *scope, StringName name, Object *value) {
    for (Scope *curr = scope; curr != NULL; curr = curr->parent) {
        for (size_t i = 0; i < curr->items.size; i++) {
            ScopeItem item = da_at(curr->items, i);
            if (item.key == name) {
                da_at(curr->items, i).value = value;
                return true;
            }
        }
    }
    return false;
}
