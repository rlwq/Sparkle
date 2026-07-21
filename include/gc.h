#ifndef GC_H
#define GC_H

#include "object.h"

#define INIT_GC_CAPACITY 64

struct GC {
    Object *objects_heap;
    size_t objects_count;

    Scope *scopes_heap;
    size_t scopes_count;

    size_t capacity;
};

GC *gc_alloc(void);
void gc_free(GC *gc);

// The GC owns the whole object lifecycle: these constructors, gc_mark_object and
// gc_free_object are the only places that know each kind's representation.
//
// gc_alloc_* never trigger a collection themselves. Collection is driven by the
// VM at safe points only (see vm_maybe_collect); this keeps the parser's
// pre-VM, not-yet-rooted allocations correct by construction.
Object *gc_alloc_object(GC *gc, ObjectKind kind);
Object *gc_alloc_integer(GC *gc, Integer value);
Object *gc_alloc_float(GC *gc, double value);
Object *gc_alloc_symbol(GC *gc, StringName value);
Object *gc_alloc_builtin(GC *gc, BuiltinObject value);
Object *gc_alloc_list(GC *gc);
Object *gc_alloc_lambda(GC *gc, bool is_variadic, Object *expr, Scope *scope);
Object *gc_alloc_string_own(GC *gc, char *data, size_t size);
Object *gc_alloc_string(GC *gc, const char *data, size_t size);

Scope *gc_alloc_scope(GC *gc, Scope *parent);
void gc_free_scope(GC *gc, Scope *scope);

bool gc_grow_if_needed(GC *gc);
void gc_mark_scope(Scope *scope);
void gc_mark_object(Object *object);
void gc_free_object(GC *gc, Object *object);
void gc_sweep(GC *gc);

#endif
