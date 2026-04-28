#ifndef GC_H
#define GC_H

#include "typedefs.h"
#include "lisp_ast.h"

struct GC {
    LispAST *nodes_heap;
    size_t nodes_count;
    
    Scope *scopes_heap;
    size_t scopes_count;
};

GC *gc_alloc(void);
void gc_free(GC *gc);

LispAST *gc_alloc_node(GC *gc, LISP_AST_KIND kind);

Scope *gc_alloc_scope(GC *gc, Scope *parent);
void gc_free_scope(GC *gc, Scope *scope);

void gc_mark_node(LispAST *expr);
void gc_free_node(GC *gc, LispAST *expr);

void gc_mark_scope(Scope *scope);

void gc_sweep(GC *gc);

#endif
