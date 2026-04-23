#include "lisp_ast.h"
#include <assert.h>

Env env_init(Env *parent) {
    Env result;

    da_init(result.symbols);
    da_init(result.values);
    result.parent = parent;

    return result;
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
        assert(0 && "No symbol found. No error handling.");
    return env_get(env->parent, expr);    
}

