#include "debug.h"
#include "forwards.h"
#include "lisp_node.h"
#include "string_view.h"
#include <assert.h>
#include <stdio.h>

bool is_proper_list(LispNode *expr) {
    for (; expr->kind == LISP_CONS; expr = CDR(expr))
        ;
    return expr->kind == LISP_NIL;
}

void print_proper_list(LispNode *expr) {
    assert(expr->kind == LISP_CONS || expr->kind == LISP_NIL);

    printf("(");

    if (expr->kind != LISP_NIL) {
        print_expr(CAR(expr));
        expr = CDR(expr);
    }
    for (; expr->kind != LISP_NIL; expr = CDR(expr)) {
        printf(" ");
        print_expr(CAR(expr));
    }

    printf(")");
}

void print_improper_list(LispNode *expr) {
    assert(expr->kind == LISP_CONS || expr->kind == LISP_NIL);

    size_t depth = 0;

    for (; expr->kind == LISP_CONS; expr = CDR(expr)) {
        printf("(cons ");
        print_expr(CAR(expr));
        printf(" ");
        depth++;
    }
    print_expr(expr);
    for (size_t i = 0; i < depth; i++)
        printf(")");
}

void print_cons(LispNode *expr) {
    assert(expr->kind == LISP_CONS);

    if (is_proper_list(expr))
        print_proper_list(expr);
    else
        print_improper_list(expr);
}

void print_lambda(LispNode *expr) {
    assert(expr->kind == LISP_LAMBDA);

    printf("(lambda (");
    if (expr->as.lambda.args.size > 0) {
        StringView curr = da_at(expr->as.lambda.args, 0);
        printf(SV_FMT, SV_ARGS(curr));
    }
    for (size_t i = 1; i < expr->as.lambda.args.size; i++) {
        StringView curr = da_at(expr->as.lambda.args, i);
        printf(" " SV_FMT, SV_ARGS(curr));
    }
    printf(") ");
    print_expr(expr->as.lambda.expr);
    printf(")");
}

void print_expr(LispNode *expr) {
    switch (expr->kind) {
    case LISP_NIL:
        printf("NIL");
        break;
    case LISP_INTEGER:
        printf("%d", expr->as.integer);
        break;
    case LISP_STRING:
        printf("\"" SV_FMT "\"", SV_ARGS(expr->as.string));
        break;
    case LISP_SYMBOL:
        printf(SV_FMT, SV_ARGS(expr->as.symbol));
        break;
    case LISP_CONS:
        print_cons(expr);
        break;
    case LISP_BUILTIN:
        break;
    case LISP_LAMBDA:
        print_lambda(expr);
        break;
    }
}
