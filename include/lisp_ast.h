#ifndef LISP_AST_H
#define LISP_AST_H

#include "string_view.h"

typedef enum {
    LISP_NIL,
    LISP_CONS,
    LISP_SYMBOL,
    LISP_INTEGER,
    LISP_STRING,
} LISP_AST_KIND;

typedef struct LispAST LispAST;

typedef struct {
    LispAST *car;
    LispAST *cdr;
} Cons;

struct LispAST {
    LISP_AST_KIND kind;
    union {
        StringView symbol;
        StringView string;
        Cons cons;
        int integer;
    } as;
};

#endif
