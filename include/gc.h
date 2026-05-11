#ifndef GC_H
#define GC_H

#include "object.h"

#define INIT_GC_CAPACITY 64

struct GC {
    Object *nodes_heap;
    size_t nodes_count;

    Scope *scopes_heap;
    size_t scopes_count;

    size_t capacity;
};

GC *gc_alloc(void);
void gc_free(GC *gc);

Object *gc_alloc_node(GC *gc, ObjectKind kind);

Scope *gc_alloc_scope(GC *gc, Scope *parent);
void gc_free_scope(GC *gc, Scope *scope);

bool gc_check_bounds(GC *gc);
void gc_mark_scope(Scope *scope);
void gc_mark_node(Object *expr);
void gc_free_node(GC *gc, Object *expr);
void gc_sweep(GC *gc);

#endif
