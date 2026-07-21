#include "gc.h"
#include "dynamic_array.h"
#include "forwards.h"
#include "object.h"
#include "scope.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

GC *gc_alloc(void) {
    GC *gc = malloc(sizeof(GC));
    assert(gc);

    gc->objects_heap = NULL;
    gc->objects_count = 0;

    gc->scopes_heap = NULL;
    gc->scopes_count = 0;

    gc->capacity = INIT_GC_CAPACITY;

    return gc;
}

void gc_free(GC *gc) {
    while (gc->objects_heap) {
        Object *next = gc->objects_heap->heap_next;
        gc_free_object(gc, gc->objects_heap);
        gc->objects_heap = next;
    }

    while (gc->scopes_heap) {
        Scope *next = gc->scopes_heap->heap_next;
        gc_free_scope(gc, gc->scopes_heap);
        gc->scopes_heap = next;
    }

    free(gc);
}

bool gc_grow_if_needed(GC *gc) {
    if (gc->scopes_count + gc->objects_count < gc->capacity)
        return false;
    gc->capacity *= 2;
    return true;
}

Object *gc_alloc_object(GC *gc, ObjectKind kind) {
    Object *object = malloc(sizeof(Object));
    assert(object);

    gc->objects_count++;
    object->kind = kind;

    object->marked = false;
    object->heap_next = gc->objects_heap;
    gc->objects_heap = object;

    return object;
}

Object *gc_alloc_integer(GC *gc, Integer value) {
    Object *object = gc_alloc_object(gc, KIND_INTEGER);
    OBJ_INTEGER(object) = value;
    return object;
}

Object *gc_alloc_float(GC *gc, double value) {
    Object *object = gc_alloc_object(gc, KIND_FLOAT);
    OBJ_FLOAT(object) = value;
    return object;
}

Object *gc_alloc_symbol(GC *gc, StringName value) {
    Object *object = gc_alloc_object(gc, KIND_SYMBOL);
    OBJ_SYMBOL(object) = value;
    return object;
}

Object *gc_alloc_builtin(GC *gc, BuiltinObject value) {
    Object *object = gc_alloc_object(gc, KIND_BUILTIN);
    OBJ_BUILTIN(object) = value;
    return object;
}

Object *gc_alloc_list(GC *gc) {
    Object *object = gc_alloc_object(gc, KIND_LIST);
    da_init(OBJ_LIST_ITEMS(object));
    return object;
}

Object *gc_alloc_lambda(GC *gc, bool is_variadic, Object *expr, Scope *scope) {
    Object *object = gc_alloc_object(gc, KIND_LAMBDA);
    da_init(OBJ_LAMBDA_ARGS(object));
    OBJ_LAMBDA_IS_VARIADIC(object) = is_variadic;
    OBJ_LAMBDA_SUBEXPR(object) = expr;
    OBJ_LAMBDA_SCOPE(object) = scope;
    return object;
}

// Adopts data: a malloc'd buffer holding at least size bytes, never
// zero-sized, that the GC will free with the object.
Object *gc_alloc_string_own(GC *gc, char *data, size_t size) {
    Object *object = gc_alloc_object(gc, KIND_STRING);
    OBJ_STRING_DATA(object) = data;
    OBJ_STRING_SIZE(object) = size;
    return object;
}

// Copies the bytes: the argument is only read, never adopted.
Object *gc_alloc_string(GC *gc, const char *data, size_t size) {
    char *buffer = malloc(size + 1);
    assert(buffer);
    memcpy(buffer, data, size);
    return gc_alloc_string_own(gc, buffer, size);
}

void gc_free_object(GC *gc, Object *object) {
    assert(!object->marked);
    gc->objects_count--;

    switch (object->kind) {
    case KIND_NIL:
    case KIND_INTEGER:
    case KIND_SYMBOL:
    case KIND_BUILTIN:
    case KIND_BOOL:
    case KIND_FLOAT:
        free(object);
        break;

    case KIND_LIST:
        da_free(OBJ_LIST_ITEMS(object));
        free(object);
        break;

    case KIND_STRING:
        free(OBJ_STRING_DATA(object));
        free(object);
        break;

    case KIND_LAMBDA:
        da_free(OBJ_LAMBDA_ARGS(object));
        free(object);
        break;
    }
}

Scope *gc_alloc_scope(GC *gc, Scope *parent) {
    Scope *scope = malloc(sizeof(Scope));
    assert(scope);

    da_init(scope->items);

    scope->marked = false;

    scope->heap_next = gc->scopes_heap;
    gc->scopes_heap = scope;
    gc->scopes_count++;

    scope->parent = parent;

    return scope;
}

void gc_free_scope(GC *gc, Scope *scope) {
    da_free(scope->items);

    gc->scopes_count--;
    free(scope);
}

void gc_sweep(GC *gc) {
    Object **curr_object = &(gc->objects_heap);

    while (*curr_object) {
        if ((*curr_object)->marked) {
            (*curr_object)->marked = false;
            curr_object = &((*curr_object)->heap_next);
        } else {
            Object *dead = *curr_object;
            *curr_object = dead->heap_next;
            gc_free_object(gc, dead);
        }
    }

    Scope **curr_scope = &(gc->scopes_heap);

    while (*curr_scope) {
        if ((*curr_scope)->marked) {
            (*curr_scope)->marked = false;
            curr_scope = &((*curr_scope)->heap_next);
        } else {
            Scope *dead = *curr_scope;
            *curr_scope = dead->heap_next;
            gc_free_scope(gc, dead);
        }
    }
}

void gc_mark_object(Object *object) {
    ObjectPtrDA to_mark;
    da_init(to_mark);

    da_push(to_mark, object);

    while (to_mark.size > 0) {
        Object *curr = da_at_end(to_mark, 0);
        assert(curr);
        da_pop(to_mark);

        if (curr->marked)
            continue;

        curr->marked = true;

        switch (curr->kind) {
        case KIND_NIL:
        case KIND_SYMBOL:
        case KIND_INTEGER:
        case KIND_STRING:
        case KIND_BUILTIN:
        case KIND_FLOAT:
        case KIND_BOOL:
            break;

        case KIND_LAMBDA:
            da_push(to_mark, OBJ_LAMBDA_SUBEXPR(curr));
            gc_mark_scope(OBJ_LAMBDA_SCOPE(curr));
            break;

        case KIND_LIST:
            for (size_t i = 0; i < OBJ_LIST_SIZE(curr); i++)
                da_push(to_mark, OBJ_LIST_AT(curr, i));
            break;
        }
    }

    da_free(to_mark);
}

void gc_mark_scope(Scope *scope) {
    while (scope && !scope->marked) {
        scope->marked = true;

        for (size_t i = 0; i < scope->items.size; i++)
            gc_mark_object(da_at(scope->items, i).value);

        scope = scope->parent;
    }
}
