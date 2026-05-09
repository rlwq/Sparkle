#ifndef LISP_NODE_H
#define LISP_NODE_H

#include <stdbool.h>
#include <stddef.h>

#include "forwards.h"
#include "string_interner.h"

typedef enum {
    LISP_NIL,
    LISP_CONS,
    LISP_SYMBOL,
    LISP_BOOL,
    LISP_FLOAT,
    LISP_INTEGER,
    LISP_STRING,
    LISP_BUILTIN,
    LISP_LAMBDA,
    LISP_EXCEPTION,
} LispNodeKind;

typedef enum {
    INVALID_SPECIAL_FORM,
    SYMBOL_REBINDING,
    SYMBOL_UNDEFINED,
    UNCALLABLE_CALL,
    WRONG_ARITY,
    WRONG_TYPE,
} ExceptionKind;

typedef struct {
    LispNode *car;
    LispNode *cdr;
} LispConsNode;

typedef DA(StringName) LambdaArgs;

typedef long long int Integer;

typedef struct {
    LambdaArgs args;
    LispNode *subexpr;
    Scope *scope;
    bool is_variadic;
} LispLambdaNode;

typedef struct {
    void (*func)(VM *vm);
    size_t arity;
    bool is_variadic;
} LispBuiltin;

struct LispNode {
    LispNodeKind kind;
    bool marked;

    LispNode *heap_next;

    union {
        StringName symbol;
        LispBuiltin builtin;
        LispLambdaNode lambda;
        LispConsNode cons;
        Integer integer;
        double float_;
        bool bool_;
        ExceptionKind exception;
    } as;
};

#define IS_NUMBERIC(n_)                                                                            \
    ((n_)->kind == LISP_FLOAT || (n_)->kind == LISP_INTEGER || (n_)->kind == LISP_BOOL)

#define CONS(n_) ((n_)->as.cons)
#define CAR(n_) ((n_)->as.cons.car)
#define CDR(n_) ((n_)->as.cons.cdr)

#define SYMBOL(n_) ((n_)->as.symbol)
#define INTEGER(n_) ((n_)->as.integer)
#define FLOAT(n_) ((n_)->as.float_)
#define BOOL(n_) ((n_)->as.bool_)
#define EXCEPTION(n_) ((n_)->as.exception)

#define LAMBDA(n_) ((n_)->as.lambda)
#define LAMBDA_POS_ARGS_N(n_) ((n_)->as.lambda.args.size - ((n_)->as.lambda.is_variadic ? 1 : 0))
#define LAMBDA_ARGS(n_) ((n_)->as.lambda.args)
#define LAMBDA_SUBEXPR(n_) ((n_)->as.lambda.subexpr)
#define LAMBDA_IS_VARIADIC(n_) ((n_)->as.lambda.is_variadic)
#define LAMBDA_SCOPE(n_) ((n_)->as.lambda.scope)

#define BUILTIN(n_) ((n_)->as.builtin)
#define BUILTIN_ARGS_N(n_) ((n_)->as.builtin.arity)

#endif
