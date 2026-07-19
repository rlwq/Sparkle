#ifndef OBJECT_H
#define OBJECT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "forwards.h"
#include "string_interner.h"

// Adding a kind: extend X_KINDS, add the union member and accessor macros
// below, then give gc.c a constructor plus mark and free cases, io.c's
// write_expr a way to print it, and vm_eval.c a way to evaluate it. Kind
// switches are written without a default, so -Wswitch-enum turns every place
// left to update into a compile error.
#define X_KINDS                                                                                    \
    X(NIL)                                                                                         \
    X(LIST)                                                                                        \
    X(SYMBOL)                                                                                      \
    X(BOOL)                                                                                        \
    X(FLOAT)                                                                                       \
    X(INTEGER)                                                                                     \
    X(STRING)                                                                                      \
    X(BUILTIN)                                                                                     \
    X(LAMBDA)

typedef enum {
#define X(k_) KIND_##k_,
    X_KINDS
#undef X
} ObjectKind;

typedef enum {
#define X(k_) TY_##k_ = 1 << KIND_##k_,
    X_KINDS
#undef X
        TY_NUMERIC = TY_BOOL | TY_INTEGER | TY_FLOAT,
    TY_CALLABLE = TY_BUILTIN | TY_LAMBDA,

    TY_ANY = 0
#define X(k_) | TY_##k_
    X_KINDS
#undef X
} ObjectType;

#define TYPEOF(n_) (1 << (n_)->kind)
#define OFTYPE(n_, t_) (TYPEOF(n_) & (t_))

typedef struct {
    DA(Object *) items;
} ListObject;

typedef long long int Integer;

typedef struct {
    DA(StringName) args;
    Object *subexpr;
    Scope *scope;
    bool is_variadic;
} LambdaObject;

typedef struct {
    void (*func)(VM *vm);
    size_t arity;
    bool is_variadic;
} BuiltinObject;

// Strings are immutable byte arrays, so sharing one object is always safe.
// data is malloc'd and at least size + 1 bytes (never zero-sized), but the
// content is not NUL-terminated and may itself contain '\0' - always work
// with the explicit size.
typedef struct {
    char *data;
    size_t size;
} StringObject;

typedef union {
    StringName symbol;
    BuiltinObject builtin;
    LambdaObject lambda;
    ListObject list;
    Integer integer;
    double float_;
    bool bool_;
    StringObject string;
} ObjectUnion;

struct Object {
    ObjectKind kind;
    bool marked;

    Object *heap_next;
    ObjectUnion as;
};

#define LIST(n_) ((n_)->as.list)
#define LIST_ITEMS(n_) ((n_)->as.list.items)
#define LIST_SIZE(n_) ((n_)->as.list.items.size)
#define LIST_AT(n_, i_) (da_at((n_)->as.list.items, (i_)))

#define SYMBOL(n_) ((n_)->as.symbol)
#define INTEGER(n_) ((n_)->as.integer)
#define FLOAT(n_) ((n_)->as.float_)
#define BOOL(n_) ((n_)->as.bool_)
#define STRING(n_) ((n_)->as.string)
#define STRING_DATA(n_) ((n_)->as.string.data)
#define STRING_SIZE(n_) ((n_)->as.string.size)

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

#define LIST_FOREACH(name_, list_)                                                                 \
    for (size_t name_##_i = 0; name_##_i < LIST_SIZE(list_); name_##_i++) {                        \
        Object *name_ = LIST_AT(list_, name_##_i);

#define END_LIST_FOREACH }

#endif
