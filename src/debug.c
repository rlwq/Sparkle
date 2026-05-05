#include "debug.h"
#include "forwards.h"
#include "lisp_node.h"
#include "string_view.h"
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
        StringView curr = da_at(expr->as.lambda.args, 0);
        printf(SV_FMT, SV_ARGS(curr));
    }
    for (size_t i = 1; i < expr->as.lambda.args.size - (expr->as.lambda.is_variadic ? 1 : 0); i++) {
        StringView curr = da_at(expr->as.lambda.args, i);
        printf(" " SV_FMT, SV_ARGS(curr));
    }
    if (expr->as.lambda.is_variadic) {
        printf(" . " SV_FMT, SV_ARGS(da_at_end(expr->as.lambda.args, 0)));
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
