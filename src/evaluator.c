#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "evaluator.h"
#include "dynamic_array.h"
#include "lisp_ast.h"
#include "string_view.h"
#include "utils.h"

LispASTPtrDA unpack_list(LispAST *expr) {
    LispASTPtrDA array;
    da_init(array);
    
    // TODO: make assertions
    for (; expr->kind != LISP_NIL; expr = CDR(expr))
        da_push(array, CAR(expr));

    return array;
}

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
            printf("( lambda (");
            for (size_t i = 0; i < expr->as.lambda.args.size; i++) {
                LispAST *curr = da_at(expr->as.lambda.args, i);
                print_expr(curr);
                printf(" ");
            }
            printf(") ");
            print_expr(expr->as.lambda.expr);
            printf(")");
        break;
    }
}

Evaluator *evaluator_alloc(LispASTPtrDA exprs) {
    Evaluator *evaluator = malloc(sizeof(Evaluator));
    
    evaluator->exprs = exprs;
    evaluator->cursor = 0;
    evaluator->global_scope = env_alloc(NULL);

    return evaluator;
}

void evaluator_free(Evaluator *evaluator) {
    env_free(evaluator->global_scope);
    free(evaluator);
}

LispAST *eval_list(LispAST *expr, Env *env) {
    assert(expr->kind == LISP_NIL || expr->kind == LISP_CONS);
    if (expr->kind == LISP_NIL) return expr;
    if (expr->kind == LISP_CONS) {
        LispAST *node = gc_alloc(LISP_CONS);
        node->as.cons.car = eval_expr(expr->as.cons.car, env);
        node->as.cons.cdr = eval_list(expr->as.cons.cdr, env);
        return node;
    }

    UNREACHABLE();
}

LispAST *eval_current(Evaluator *evaluator) {
    assert(evaluator->cursor < evaluator->exprs.size);
    LispAST *node = eval_expr(da_at(evaluator->exprs, evaluator->cursor),
                              evaluator->global_scope);
    evaluator->cursor++;
    return node;
}

void register_builtin(Evaluator *evaluator, StringView name, LispBuiltin func_ptr) {
    LispAST *builtin = gc_alloc(LISP_BUILTIN);
    builtin->as.builtin = func_ptr;

    env_define(evaluator->global_scope, name, builtin);
}

void eval_all(Evaluator *evaluator) {
    while (evaluator->cursor < evaluator->exprs.size) {
        LispAST *node = eval_current(evaluator);
        print_expr(node);
        printf("\n");
    }
}

LispAST *eval_let_form(LispAST *symbol, LispAST *expr, Env *env) {
    assert(symbol->kind == LISP_SYMBOL);
    LispAST *node = eval_expr(expr, env);
    env_define(env, symbol->as.symbol, node);
    return node;
}

LispAST *eval_if_form(LispAST *condition, LispAST *if_true, LispAST *if_false, Env *env) {
    LispAST *expr_result = eval_expr(condition, env);
    if (expr_result->kind == LISP_NIL)
        return eval_expr(if_false, env); 
    return eval_expr(if_true, env);
}

LispAST *eval_lambda_form(LispASTPtrDA args, LispAST *subexpr, Env *env) {
    LispAST *lambda_result = gc_alloc(LISP_LAMBDA);

    lambda_result->as.lambda.args = args;
    lambda_result->as.lambda.expr = subexpr;
    lambda_result->as.lambda.env = env;

    return lambda_result;
}

LispAST *eval_lambda_call(LispAST *lambda, LispAST *args) {
    Env *local_scope = env_alloc(lambda->as.lambda.env);
    
    for (size_t i = 0; args->kind != LISP_NIL; args = args->as.cons.cdr) {
        env_define(local_scope, da_at(lambda->as.lambda.args, i)->as.symbol,
                   args->as.cons.car);
        i++;
    }

    LispAST *result = eval_expr(lambda->as.lambda.expr, local_scope);
    
    // env_free(local_scope);
    // TODO: should manage allocated scopes

    return result;
}

LispAST *eval_sexpr(LispAST *expr, Env *env) {
    assert(expr->kind == LISP_CONS);

    LispAST *head = expr->as.cons.car; 
    LispAST *args = expr->as.cons.cdr; 
 
    if (head->kind == LISP_SYMBOL) {
        // TODO: sv_eq to something else
        if (sv_eq(head->as.symbol, sv_mk("if"))) {
            //TODO: make proper assertions
            LispAST *condition = CAR(args);
            LispAST *if_true = CAR(CDR(args));
            LispAST *if_false = CAR(CDR(CDR(args)));

            return eval_if_form(condition, if_true, if_false, env);
        }

        else if (sv_eq(head->as.symbol, sv_mk("let"))) {
            assert(args->kind == LISP_CONS);

            LispAST *symbol = args->as.cons.car;
            LispAST *value_cons = args->as.cons.cdr;

            assert(symbol->kind == LISP_SYMBOL);
            assert(value_cons->kind == LISP_CONS);
            assert(value_cons->as.cons.cdr->kind == LISP_NIL);

            return eval_let_form(symbol, value_cons->as.cons.car, env);
        }

        else if (sv_eq(head->as.symbol, sv_mk("lambda"))) {
            // TODO: make proper assertions
            LispASTPtrDA lambda_args = unpack_list(CAR(args));
            LispAST *lambda_subexpr = CAR(CDR(args));
            LispAST *result = eval_lambda_form(lambda_args, lambda_subexpr, env);
            return result;
        }
    }

    LispAST *evaluated_head = eval_expr(head, env);
    LispAST *evaluated_args = eval_list(args, env);

    switch (evaluated_head->kind) {
        case LISP_LAMBDA:
            return eval_lambda_call(evaluated_head, evaluated_args);
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
        case LISP_LAMBDA:
            return expr;
        break;

        case LISP_SYMBOL:
            //TODO: maybe should be a separate token
            if (sv_eq(expr->as.symbol, sv_mk("NIL")))
                return gc_alloc(LISP_NIL);
            return env_get(env, expr);
        break;

        case LISP_CONS:
            return eval_sexpr(expr, env);
        break;
    }
    
    UNREACHABLE();
}

