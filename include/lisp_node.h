#ifndef LISP_NODE_H
#define LISP_NODE_H

#include <stdbool.h>
#include <stddef.h>

#include "forwards.h"
#include "string_view.h"

typedef enum {
    LISP_NIL,
    LISP_CONS,
    LISP_SYMBOL,
    LISP_INTEGER,
    LISP_STRING,
    LISP_BUILTIN,
    LISP_LAMBDA,
} LispNodeKind;

typedef struct {
    LispNode *car;
    LispNode *cdr;
} LispConsNode;

typedef struct {
    StringViewDA args; 
    LispNode *expr;
    Scope *scope;
} LispLambdaNode;

typedef void (*LispBuiltin) (VM *vm, size_t args_count);

struct LispNode {
    LispNodeKind kind;
    bool marked;

    LispNode *heap_next;
    
    union {
        StringView symbol;
        StringView string;
        LispBuiltin builtin;
        LispLambdaNode lambda;
        LispConsNode cons;
        int integer;
    } as;
};

#define CAR(n_) ((n_)->as.cons.car)
#define CDR(n_) ((n_)->as.cons.cdr)

#endif
