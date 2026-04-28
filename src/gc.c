#include "gc.h"
#include "dynamic_array.h"
#include "lisp_ast.h"
#include "utils.h"
#include "scope.h"

#include <assert.h>
#include <stdlib.h>


GC *gc_alloc(void) {
    GC *gc = malloc(sizeof(GC));
    assert(gc);

    return gc;
}

void gc_free(GC *gc) {
    // TODO: free all objects in heap 
    free(gc);
}

// TODO: split allocation/deallocation logic with construction/deconstruction logic
LispAST *gc_alloc_node(GC *gc, LISP_AST_KIND kind) {
    LispAST *node = malloc(sizeof(LispAST));
    assert(node); //TODO: add some error reporting
    
    gc->nodes_count++;
    node->kind = kind;

    node->marked = false;
    node->heap_next = gc->nodes_heap;
    gc->nodes_heap = node;

    return node;
}

Scope *gc_alloc_scope(GC *gc, Scope *parent) {
    Scope *scope = malloc(sizeof(Scope));
    assert(scope);

    da_init(scope->symbols);
    da_init(scope->values);

    scope->heap_next = gc->scopes_heap;
    gc->scopes_heap = scope;
    gc->scopes_count++;

    scope->parent = parent;

    return scope;
}

void gc_free_node(GC *gc, LispAST *expr) {
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

void gc_sweep(GC *gc) {
    LispAST **curr = &(gc->nodes_heap);

    while (*curr) {
        if ((*curr)->marked) {
            (*curr)->marked = false;
            curr = &((*curr)->heap_next);
        }
        else {
            LispAST *dead = *curr;
            *curr = dead->heap_next;
            gc_free_node(gc, dead);
        }
    }
}

void gc_mark_node(LispAST *expr) {
    assert(expr);
    
    if (expr->marked) return;

    expr->marked = true;

    switch (expr->kind) {
        case LISP_NIL:
        case LISP_SYMBOL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_BUILTIN:
            // Do nothin
        break;

        case LISP_LAMBDA:
            NOT_IMPLEMENTED();
        break;

        case LISP_CONS:
            gc_mark_node(expr->as.cons.car);
            gc_mark_node(expr->as.cons.cdr);
        break;
    }
}


