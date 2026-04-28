#ifndef LISP_AST_H
#define LISP_AST_H

#include "string_view.h"
#include "typedefs.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    LISP_NIL,
    LISP_CONS,
    LISP_SYMBOL,
    LISP_INTEGER,
    LISP_STRING,
    LISP_BUILTIN,
    LISP_LAMBDA,
} LISP_AST_KIND;

typedef struct {
    LispAST *car;
    LispAST *cdr;
} Cons;

typedef struct {
    LispASTPtrDA args; 
    LispAST *expr;
    Scope *scope;
} Lambda;

typedef LispAST *(*LispBuiltin) (LispAST *args, GC *gc);

struct LispAST {
    LISP_AST_KIND kind;
    bool marked;

    LispAST *heap_next;
    
    union {
        StringView symbol;
        StringView string;
        LispBuiltin builtin;
        Lambda lambda;
        Cons cons;
        int integer;
    } as;
};

#define CAR(n_) ((n_)->as.cons.car)
#define CDR(n_) ((n_)->as.cons.cdr)

#endif
