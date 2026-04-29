#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "gc.h"
#include "dynamic_array.h"
#include "lisp_node.h"
#include "scope.h"
#include "forwards.h"

GC *gc_alloc(void) {
    GC *gc = malloc(sizeof(GC));
    assert(gc);

    gc->nodes_heap = NULL;
    gc->nodes_count = 0;

    gc->scopes_heap = NULL;
    gc->scopes_count = 0;

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

// TODO: split allocation/deallocation logic with construction/deconstruction logic
LispNode *gc_alloc_node(GC *gc, LispNodeKind kind) {
    LispNode *node = malloc(sizeof(LispNode));
    assert(node); //TODO: add some error reporting 
      
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

    da_init(scope->symbols);
    da_init(scope->values);

    scope->marked = false;

    scope->heap_next = gc->scopes_heap;
    gc->scopes_heap = scope;
    gc->scopes_count++;
    
    scope->parent = parent;

    return scope;
}

void gc_free_scope(GC *gc, Scope *scope) {
    da_free(scope->symbols);
    da_free(scope->values);
    
    gc->scopes_count--;
    free(scope);
}

void gc_sweep(GC *gc) {
    printf("OBJECTS BEFORE CLEANUP: %zu\n",
           gc->scopes_count + gc->nodes_count);

    LispNode **curr_node = &(gc->nodes_heap);

    while (*curr_node) {
        if ((*curr_node)->marked) {
            (*curr_node)->marked = false;
            curr_node = &((*curr_node)->heap_next);
        }
        else {
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
        }
        else {
            Scope *dead = *curr_scope;
            *curr_scope = dead->heap_next;
            gc_free_scope(gc, dead);
        }
    }

    printf("OBJECTS  AFTER CLEANUP: %zu\n\n",
           gc->scopes_count + gc->nodes_count);
}

void gc_mark_node(LispNode *expr) {
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
            gc_mark_node(expr->as.lambda.expr);
            gc_mark_scope(expr->as.lambda.scope);
        break;

        case LISP_CONS:
            gc_mark_node(expr->as.cons.car);
            gc_mark_node(expr->as.cons.cdr);
        break;
    }
}

void gc_mark_scope(Scope *scope) {
    assert(scope);

    scope->marked = true;
   
    for (size_t i = 0; i < scope->values.size; i++)
        gc_mark_node(da_at(scope->values, i));

    if (scope->parent)
        gc_mark_scope(scope->parent);
}

