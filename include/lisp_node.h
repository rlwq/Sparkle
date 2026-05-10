#ifndef LISP_NODE_H
#define LISP_NODE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "forwards.h"
#include "string_interner.h"

#define X_KINDS                                                                                    \
    X(NIL)                                                                                         \
    X(CONS)                                                                                        \
    X(SYMBOL)                                                                                      \
    X(BOOL)                                                                                        \
    X(FLOAT)                                                                                       \
    X(INTEGER)                                                                                     \
    X(STRING)                                                                                      \
    X(BUILTIN)                                                                                     \
    X(LAMBDA)                                                                                      \
    X(EXCEPTION)

typedef enum {
#define X(k_) LISP_##k_,
    X_KINDS
#undef X
} LispNodeKind;

typedef enum {
#define X(k_) TY_##k_ = 1 << LISP_##k_,
    X_KINDS
#undef X
        TY_NUMERIC = TY_BOOL | TY_INTEGER | TY_FLOAT,
    TY_LISTFUL = TY_CONS | TY_NIL,
    TY_CALLABLE = TY_BUILTIN | TY_LAMBDA,
} LispNodeType;

#define TYPEOF(n_) (1 << (n_)->kind)
#define OFTYPE(n_, t_) (TYPEOF(n_) & (t_))

typedef enum {
    INVALID_SPECIAL_FORM,
    SYMBOL_REBINDING,
    SYMBOL_UNDEFINED,
    UNCALLABLE_CALL,
    WRONG_ARITY,
    WRONG_TYPE,
    WRONG_VALUE,
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

typedef union {
    StringName symbol;
    LispBuiltin builtin;
    LispLambdaNode lambda;
    LispConsNode cons;
    Integer integer;
    double float_;
    bool bool_;
    ExceptionKind exception;
} LispNodeUnion;

struct LispNode {
    LispNodeKind kind;
    bool marked;

    LispNode *heap_next;
    LispNodeUnion as;
};

#define IS_NUMBERIC(n_)                                                                            \
    ((n_)->kind == LISP_FLOAT || (n_)->kind == LISP_INTEGER || (n_)->kind == LISP_BOOL)

#define NODE_IS(n_, k_) ((n_)->kind == (k_))

#define IS_LISTFUL(n_) ((n_)->kind == LISP_CONS || (n_)->kind == LISP_NIL)

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
#define BUILTIN_FUNC(n_) ((n_)->as.builtin.func)
#define BUILTIN_IS_VARIADIC(n_) ((n_)->as.builtin.is_variadic)
#define BUILTIN_ARGS_N(n_) ((n_)->as.builtin.arity)

#define LIST_ITER(vm_, name_, value_)                                                              \
    LispNode *name_;                                                                               \
    for (name_ = value_; OFTYPE(name_, TY_CONS); name_ = CDR(name_)) {

#define END_LIST_ITER(vm_, name_)                                                                  \
    }                                                                                              \
    VM_RECOVER_IF(vm_, !OFTYPE(name_, TY_NIL), WRONG_TYPE);

#endif
