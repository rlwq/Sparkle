#include <assert.h>
#include <stdlib.h>

#include "gc.h"
#include "scope.h"
#include "evaluator.h"
#include "dynamic_array.h"
#include "lisp_ast.h"
#include "string_view.h"
#include "utils.h"

#define CURR(e_) (*((e_)->stmts))

Evaluator *evaluator_alloc(LispASTPtrDA exprs, GC *gc) {
    Evaluator *evaluator = malloc(sizeof(Evaluator));
    assert(evaluator);

    evaluator->stmts = exprs.data;
    evaluator->stmts_count = exprs.size;
    evaluator->is_err = false;

    evaluator->gc = gc;

    da_init(evaluator->results);
    da_init(evaluator->scope_stack);

    return evaluator;
}

void evaluator_free(Evaluator *evaluator) {
    free(evaluator);
}

void push_scope(Evaluator *evaluator, Scope *scope) {
    da_push(evaluator->scope_stack, scope);
}

void pop_scope(Evaluator *evaluator) {
    da_pop(evaluator->scope_stack);
}

LispAST *evaluator_advance(Evaluator *evaluator) {
    EVALUATOR_VALID(evaluator);
    LispAST *curr = CURR(evaluator);
    
    evaluator->stmts++;
    evaluator->stmts_count--;
    return curr;
}

LispAST *eval_expr(LispAST *expr, Evaluator *evaluator);

LispASTPtrDA unpack_lisp_list(LispAST *expr) {
    LispASTPtrDA array;
    da_init(array);

    for (; expr->kind != LISP_NIL; expr = CDR(expr)) {
        assert(expr->kind == LISP_CONS);
        da_push(array, CAR(expr));
    }

    return array;
}
//
// void evaluator_mark_stmts(Evaluator *evaluator) {
//     scope_mark(CURR_SCOPE(evaluator));
//     for (size_t i = 0; i < evaluator->stmts_count; i++)
//         gc_mark_node(evaluator->stmts[i]);
// }
//
LispAST *eval_list(LispAST *expr, Evaluator *evaluator) {
    assert(expr->kind == LISP_NIL || expr->kind == LISP_CONS);

    // TODO: make an iterative approach
    if (expr->kind == LISP_NIL) return expr;
    if (expr->kind == LISP_CONS) {
        LispAST *node = gc_alloc_node(evaluator->gc, LISP_CONS);
        node->as.cons.car = eval_expr(expr->as.cons.car, evaluator);
        node->as.cons.cdr = eval_list(expr->as.cons.cdr, evaluator);
        return node;
    }

    UNREACHABLE();
}

void eval_current(Evaluator *evaluator) {
    assert(EVALUATOR_VALID(evaluator));
    
    LispAST *stmt = evaluator_advance(evaluator);
    LispAST *node = eval_expr(stmt, evaluator);

    if (node) da_push(evaluator->results, node);
}

void register_builtin(Evaluator *evaluator, StringView name, LispBuiltin func_ptr) {
    LispAST *builtin = gc_alloc_node(evaluator->gc, LISP_BUILTIN);
    builtin->as.builtin = func_ptr;

    scope_define(CURR_SCOPE(evaluator), name, builtin);
}

void eval_all(Evaluator *evaluator) {
    assert(EVALUATOR_VALID(evaluator));

    while (EVALUATOR_VALID(evaluator))
        eval_current(evaluator);
}

LispAST *eval_let_form(LispAST *symbol, LispAST *expr, Evaluator *evaluator) {
    assert(symbol->kind == LISP_SYMBOL);
    LispAST *node = eval_expr(expr, evaluator);
    scope_define(CURR_SCOPE(evaluator), symbol->as.symbol, node);
    return node;
}

LispAST *eval_if_form(LispAST *condition, LispAST *if_true, LispAST *if_false, Evaluator *evaluator) {
    LispAST *expr_result = eval_expr(condition, evaluator);
    if (expr_result->kind == LISP_NIL)
        return eval_expr(if_false, evaluator); 
    return eval_expr(if_true, evaluator);
}

LispAST *eval_lambda_form(LispASTPtrDA args, LispAST *subexpr, Evaluator *evaluator) {
    LispAST *lambda_result = gc_alloc_node(evaluator->gc, LISP_LAMBDA);

    lambda_result->as.lambda.args = args;
    lambda_result->as.lambda.expr = subexpr;
    lambda_result->as.lambda.scope = CURR_SCOPE(evaluator);

    return lambda_result;
}

LispAST *eval_lambda_call(LispAST *lambda, LispAST *args, Evaluator *evaluator) {
    // Scope *local_scope = scope_alloc(lambda->as.lambda.scope);
    
    push_scope(evaluator, lambda->as.lambda.scope);
    push_scope(evaluator, gc_alloc_scope(evaluator->gc, CURR_SCOPE(evaluator)));

    for (size_t i = 0; args->kind != LISP_NIL; args = args->as.cons.cdr) {
        scope_define(CURR_SCOPE(evaluator), da_at(lambda->as.lambda.args, i)->as.symbol,
                   args->as.cons.car);
        i++;
    }

    LispAST *result = eval_expr(lambda->as.lambda.expr, evaluator);
    pop_scope(evaluator);
    pop_scope(evaluator);
    // scope_free(local_scope);
    // TODO: should manage allocated scopes

    return result;
}

LispAST *dispatch_special_form(LispAST *head, LispAST *args, Evaluator *evaluator) {
    if (head->kind != LISP_SYMBOL) return NULL;

    if (sv_eq(head->as.symbol, sv_mk("if"))) {
        //TODO: make proper assertions
        LispAST *condition = CAR(args);
        LispAST *if_true = CAR(CDR(args));
        LispAST *if_false = CAR(CDR(CDR(args)));

        return eval_if_form(condition, if_true, if_false, evaluator);
    }

    if (sv_eq(head->as.symbol, sv_mk("let"))) {
        assert(args->kind == LISP_CONS);

        LispAST *symbol = args->as.cons.car;
        LispAST *value_cons = args->as.cons.cdr;

        assert(symbol->kind == LISP_SYMBOL);
        assert(value_cons->kind == LISP_CONS);
        assert(value_cons->as.cons.cdr->kind == LISP_NIL);

        return eval_let_form(symbol, value_cons->as.cons.car, evaluator);
    }

    if (sv_eq(head->as.symbol, sv_mk("lambda"))) {
        // TODO: make proper assertions
        LispASTPtrDA lambda_args = unpack_lisp_list(CAR(args));
        LispAST *lambda_subexpr = CAR(CDR(args));
        LispAST *result = eval_lambda_form(lambda_args, lambda_subexpr, evaluator);
        return result;
    }

    return NULL;
}

LispAST *eval_cons(LispAST *expr, Evaluator *evaluator) {
    assert(expr->kind == LISP_CONS);

    LispAST *head = expr->as.cons.car; 
    LispAST *args = expr->as.cons.cdr; 

    LispAST *special_form = dispatch_special_form(head, args, evaluator);

    if (special_form) return special_form;

    LispAST *evaluated_head = eval_expr(head, evaluator);
    LispAST *evaluated_args = eval_list(args, evaluator);

    switch (evaluated_head->kind) {
        case LISP_LAMBDA:
            return eval_lambda_call(evaluated_head, evaluated_args, evaluator);
        break;

        case LISP_BUILTIN:
            return evaluated_head->as.builtin(evaluated_args, evaluator->gc);
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

LispAST *eval_expr(LispAST *expr, Evaluator *evaluator) {
    switch (expr->kind) {
        case LISP_NIL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_BUILTIN:
        case LISP_LAMBDA:
            return expr;
        break;

        case LISP_SYMBOL:
            if (sv_eq(expr->as.symbol, sv_mk("NIL")))
                return gc_alloc_node(evaluator->gc, LISP_NIL);
            return scope_get(CURR_SCOPE(evaluator), expr);
        break;

        case LISP_CONS:
            return eval_cons(expr, evaluator);
        break;
    }
    
    UNREACHABLE();
}

LispASTPtrDA extract_results(Evaluator *evaluator) {
    LispASTPtrDA results = evaluator->results;
    da_nullify(evaluator->results);
    return results;
}

