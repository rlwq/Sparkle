#include "evaluator.h"
#include "dynamic_array.h"
#include "lisp_ast.h"
#include "string_view.h"
#include "utils.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

void evaluator_mark(Evaluator *evaluator) {
    env_mark(evaluator->global_scope);

    //TODO: maybe should mark only the not yet evaluated ones
    for (size_t i = 0; i < evaluator->exprs.size; i++) {
        gc_mark(da_at(evaluator->exprs, i));
    }
}

void print_expr(LispAST *expr) {
    switch (expr->kind) {
        case LISP_NIL: printf("NIL"); break;
        case LISP_INTEGER: printf("%d", expr->as.integer); break;
        case LISP_STRING: printf("\""SV_FMT"\"", SV_ARGS(expr->as.string)); break;
        case LISP_SYMBOL: printf(SV_FMT, SV_ARGS(expr->as.symbol)); break;
        case LISP_CONS:
            printf("<");
            print_expr(expr->as.cons.car);
            printf("; ");
            print_expr(expr->as.cons.cdr);
            printf(">");
        break;
        case LISP_BUILTIN:
        break;
        case LISP_LAMBDA:
        break;
    }
}

Evaluator *evaluator_alloc(da_list_ast_ptr exprs) {
    Evaluator *evaluator = malloc(sizeof(Evaluator));
    
    evaluator->exprs = exprs;
    evaluator->cursor = 0;
    evaluator->global_scope = env_alloc(NULL);

    return evaluator;
}

LispAST *eval_list(LispAST *expr, Env *env) {
    assert(expr->kind == LISP_NIL || expr->kind == LISP_CONS);
    if (expr->kind == LISP_NIL) return expr;
    if (expr->kind == LISP_CONS) {
        LispAST *result = gc_alloc(LISP_CONS);
        result->as.cons.car = eval_expr(expr->as.cons.car, env);
        result->as.cons.cdr = eval_list(expr->as.cons.cdr, env);
        return result;
    }

    UNREACHABLE();
}

LispAST *evaluate_current(Evaluator *evaluator) {
    assert(evaluator->cursor < evaluator->exprs.size);
    LispAST *result = eval_expr(da_at(evaluator->exprs, evaluator->cursor),
                                evaluator->global_scope);
    evaluator->cursor++;
    return result;
}

void register_builtin(Evaluator *evaluator, StringView name, LispBuiltin func_ptr) {
    LispAST *builtin = gc_alloc(LISP_BUILTIN);
    builtin->as.builtin = func_ptr;

    env_define(evaluator->global_scope, name, builtin);
}

void evaluate_all(Evaluator *evaluator) {
    while (evaluator->cursor < evaluator->exprs.size) {
        LispAST *result = evaluate_current(evaluator);
        print_expr(result);
        printf("\n");
    }
}

LispAST *lisp_eval_let_expr(LispAST *args, Env *env) {
    assert(args->kind == LISP_CONS);
    assert(args->as.cons.car->kind == LISP_SYMBOL);
    env_define(env, args->as.cons.car->as.symbol, eval_expr(args->as.cons.cdr->as.cons.car, env));
    return args;
}

LispAST *lisp_eval_sexpr(LispAST *expr, Env *env) {
    assert(expr->kind == LISP_CONS);

    LispAST *head = expr->as.cons.car; 
    LispAST *args = expr->as.cons.cdr; 
 
    if (head->kind == LISP_SYMBOL) {
        // TODO: sv_eq to something else
        if (sv_eq(head->as.symbol, sv_mk("if"))) {
            NOT_IMPLEMENTED();
            return expr;
        }
        else if (sv_eq(head->as.symbol, sv_mk("let"))) {
            lisp_eval_let_expr(args, env);
            return expr;
        }
    }

    LispAST *evaluated_head = eval_expr(head, env);
    LispAST *evaluated_args = eval_list(args, env);

    switch (evaluated_head->kind) {
        case LISP_LAMBDA:
            NOT_IMPLEMENTED();
        break;

        case LISP_BUILTIN:
            return evaluated_head->as.builtin(evaluated_args);
        break;

        case LISP_CONS:
        case LISP_SYMBOL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_NIL:
            assert(0 && "Can't call this kind of object.");
        break;
    }

    UNREACHABLE();
}

LispAST *eval_expr(LispAST *expr, Env *env) {
    switch (expr->kind) {
        case LISP_NIL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_BUILTIN:
            return expr;
        break;

        case LISP_LAMBDA:
            NOT_IMPLEMENTED();
        break;

        case LISP_SYMBOL:
            return env_get(env, expr);
        break;

        case LISP_CONS:
            return lisp_eval_sexpr(expr, env);
        break;
    }
    
    UNREACHABLE();
}
