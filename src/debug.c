#include "debug.h"
#include "forwards.h"
#include "lisp_node.h"
#include <assert.h>
#include <stdio.h>

void print_cons(LispNode *expr) {
    assert(expr->kind == LISP_CONS);

    printf("(");
    for (; CDR(expr)->kind == LISP_CONS; expr = CDR(expr)) {
        print_expr(CAR(expr));
        printf(" ");
    }
    print_expr(CAR(expr));
    expr = CDR(expr);
    if (expr->kind != LISP_NIL) {
        printf(" . ");
        print_expr(expr);
    }
    printf(")");
}

void print_lambda(LispNode *expr) {
    assert(expr->kind == LISP_LAMBDA);

    printf("(lambda (");
    if (expr->as.lambda.args.size > (expr->as.lambda.is_variadic ? 1 : 0)) {
        StringName curr = da_at(expr->as.lambda.args, 0);
        printf("%s", curr);
    }
    for (size_t i = 1; i < LAMBDA_POS_ARGS_N(expr); i++) {
        StringName curr = da_at(expr->as.lambda.args, i);
        printf(" %s", curr);
    }
    if (expr->as.lambda.is_variadic) {
        printf(" . %s", da_at_end(expr->as.lambda.args, 0));
    }
    printf(") ");
    print_expr(expr->as.lambda.subexpr);
    printf(")");
}

void print_exception(LispNode *expr) {
    assert(expr->kind == LISP_EXCEPTION);
    printf("<EXCEPTION: ");
    switch (expr->as.exception) {
    case INVALID_SPECIAL_FORM:
        printf("INVALID_SPECIAL_FORM");
        break;
    case SYMBOL_REBINDING:
        printf("SYMBOL_REBINDING");
        break;
    case SYMBOL_UNDEFINED:
        printf("SYMBOL_UNDEFINED");
        break;
    case UNCALLABLE_CALL:
        printf("UNCALLABLE_CALL");
        break;
    case WRONG_ARITY:
        printf("WRONG_ARITY");
        break;
    case WRONG_TYPE:
        printf("WRONG_TYPE");
        break;
    }
    printf(">");
}

void print_expr(LispNode *expr) {
    switch (expr->kind) {
    case LISP_NIL:
        printf("NIL");
        break;
    case LISP_INTEGER:
        printf("%lld", expr->as.integer);
        break;
    case LISP_STRING:
        printf("NOT IMPLEMENTED");
        break;
    case LISP_SYMBOL:
        printf("%s", SYMBOL(expr));
        break;
    case LISP_CONS:
        print_cons(expr);
        break;
    case LISP_BUILTIN:
        break;
    case LISP_LAMBDA:
        print_lambda(expr);
        break;
    case LISP_EXCEPTION:
        print_exception(expr);
        break;
    }
}
