#include "lisp_ast.h"
#include "dynamic_array.h"
#include "utils.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static LispAST *ast_heap = NULL;
static size_t ast_heap_size = 0;

size_t heap_size() {
    return ast_heap_size;
}

LispAST *gc_alloc(LISP_AST_KIND kind) {
    LispAST *node = malloc(sizeof(LispAST));
    assert(node); //TODO: add some error reporting
    
    ast_heap_size++;
    node->kind = kind;

    node->marked = false;
    node->next = ast_heap;
    ast_heap = node;

    return node;
}

void gc_free(LispAST *expr) {
    assert(expr->marked);
    ast_heap_size--;
    free(expr);
}

void gc_sweep() {
    LispAST **curr = &ast_heap;

    while (*curr) {
        if ((*curr)->marked) {
            (*curr)->marked = false;
            curr = &((*curr)->next);
        }
        else {
            LispAST *dead = *curr;
            *curr = dead->next;
            gc_free(dead);
        }
    }
}

void gc_mark(LispAST *expr) {
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
            NOT_IMPLEMENTED();
        break;

        case LISP_CONS:
            gc_mark(expr->as.cons.car);
            gc_mark(expr->as.cons.cdr);
        break;
    }
}

Env* env_alloc(Env *parent) {
    Env *result = malloc(sizeof(Env));
    assert(result);

    da_init(result->symbols);
    da_init(result->values);
    result->parent = parent;

    return result;
}

Env *env_free(Env *env) {
    if (!env) return NULL;
    Env *parent = env->parent;
    
    da_free(env->symbols);
    da_free(env->values);

    return parent;
}

void env_define(Env *env, StringView name, LispAST *value) {
    da_push(env->symbols, name);
    da_push(env->values, value);
}

LispAST *env_get(Env *env, LispAST *expr) {
    assert(expr->kind == LISP_SYMBOL);
    for (size_t i = 0; i < env->symbols.size; i++) {
        if (sv_eq(da_at(env->symbols, i), expr->as.symbol))
            return da_at(env->values, i);
    }
    if (env->parent == NULL)
        assert(0 && "No symbol found. No error handling."); //TODO: error reporting
    return env_get(env->parent, expr);    
}

void env_mark(Env *env) {
    if (!env) return;

    for (size_t i = 0; i < env->symbols.size; i++)
        gc_mark(da_at(env->values, i));
    env_mark(env->parent);
}
