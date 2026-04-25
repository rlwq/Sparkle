#ifndef LISP_AST_H
#define LISP_AST_H

#include "string_view.h"
#include "dynamic_array.h"
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

typedef struct LispAST LispAST;
typedef DA(LispAST *) da_lisp_ast_ptr;
typedef struct Env Env;

typedef struct {
    LispAST *car;
    LispAST *cdr;
} Cons;

typedef struct {
    da_lisp_ast_ptr args; 
    LispAST *expr;
    Env *env;
} Lambda;

typedef LispAST *(*LispBuiltin) (LispAST *args);

struct LispAST {
    LISP_AST_KIND kind;
    bool marked;
    LispAST *next;
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

LispAST *gc_alloc(LISP_AST_KIND kind);
void gc_mark(LispAST *expr);
void gc_free(LispAST *expr);
void gc_sweep();

struct Env {
    Env *parent;
    DA(StringView) symbols;
    DA(LispAST *) values;
};

size_t heap_size();
Env* env_alloc(Env *parent);
Env* env_free(Env *env);
void env_mark(Env *env);

void env_define(Env *env, StringView name, LispAST *value);

LispAST *env_get(Env *env, LispAST *expr);

#endif
