#ifndef SCOPE_H
#define SCOPE_H

#include "lisp_ast.h"

typedef struct Scope Scope;

struct Scope {
    Scope *parent;
    
    DA(StringView) symbols;
    DA(LispAST *) values;
    
    Scope *heap_next;
    bool marked;
};

void scope_mark(Scope *scope);
void scope_define(Scope *scope, StringView name, LispAST *value);
LispAST *scope_get(Scope *scope, LispAST *expr);

#endif
