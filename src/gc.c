#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "dynamic_array.h"
#include "forwards.h"
#include "gc.h"
#include "lisp_node.h"
#include "scope.h"

GC *gc_alloc(void) {
    GC *gc = malloc(sizeof(GC));
    assert(gc);

    gc->nodes_heap = NULL;
    gc->nodes_count = 0;

    gc->scopes_heap = NULL;
    gc->scopes_count = 0;

    gc->capacity = INIT_GC_CAPACITY;

    return gc;
}

void gc_free(GC *gc) {
    while (gc->nodes_heap) {
        LispNode *next = gc->nodes_heap->heap_next;
        gc_free_node(gc, gc->nodes_heap);
        gc->nodes_heap = next;
    }

    while (gc->scopes_heap) {
        Scope *next = gc->scopes_heap->heap_next;
        gc_free_scope(gc, gc->scopes_heap);
        gc->scopes_heap = next;
    }

    free(gc);
}

bool gc_check_bounds(GC *gc) {
    if (gc->scopes_count + gc->nodes_count < gc->capacity)
        return false;
    gc->capacity *= 2;
    return true;
}

LispNode *gc_alloc_node(GC *gc, LispNodeKind kind) {
    LispNode *node = malloc(sizeof(LispNode));
    assert(node); // TODO: add some error reporting

    gc->nodes_count++;
    node->kind = kind;

    node->marked = false;
    node->heap_next = gc->nodes_heap;
    gc->nodes_heap = node;

    return node;
}

void gc_free_node(GC *gc, LispNode *expr) {
    assert(!expr->marked);
    gc->nodes_count--;

    switch (expr->kind) {
    case LISP_NIL:
    case LISP_INTEGER:
    case LISP_CONS:
    case LISP_SYMBOL:
    case LISP_BUILTIN:
    case LISP_STRING:
        free(expr);
        break;

    case LISP_LAMBDA:
        da_free(expr->as.lambda.args);
        free(expr);
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
    LispNode **curr_node = &(gc->nodes_heap);

    while (*curr_node) {
        if ((*curr_node)->marked) {
            (*curr_node)->marked = false;
            curr_node = &((*curr_node)->heap_next);
        } else {
            LispNode *dead = *curr_node;
            *curr_node = dead->heap_next;
            gc_free_node(gc, dead);
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

void gc_mark_node(LispNode *expr) {
    LispNodePtrDA to_mark;
    da_init(to_mark);

    da_push(to_mark, expr);

    while (to_mark.size > 0) {
        LispNode *curr = da_at_end(to_mark, 0);
        assert(curr);
        da_pop(to_mark);

        if (curr->marked)
            continue;

        curr->marked = true;

        switch (curr->kind) {
            case LISP_NIL:
            case LISP_SYMBOL:
            case LISP_INTEGER:
            case LISP_STRING:
            case LISP_BUILTIN:
                break;

            case LISP_LAMBDA:
                da_push(to_mark, LAMBDA_SUBEXPR(curr));
                gc_mark_scope(LAMBDA_SCOPE(curr));
                break;

            case LISP_CONS:
                da_push(to_mark, CAR(curr));
                da_push(to_mark, CDR(curr));
                break;
        }
    }

    da_free(to_mark);
}

void gc_mark_scope(Scope *scope) {
    while (scope && !scope->marked) {
        scope->marked = true;

        for (size_t i = 0; i < scope->items.size; i++)
            gc_mark_node(da_at(scope->items, i).value);

        scope = scope->parent;
    }
}

