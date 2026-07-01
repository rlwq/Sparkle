#include "io.h"
#include "forwards.h"
#include "object.h"
#include <assert.h>
#include <stdio.h>

void print_list(Object *expr) {
    assert(expr->kind == KIND_LIST);

    printf("(");
    for (size_t i = 0; i < LIST_SIZE(expr); i++) {
        if (i > 0)
            printf(" ");
        print_expr(LIST_AT(expr, i));
    }
    printf(")");
}

void print_lambda(Object *expr) {
    assert(expr->kind == KIND_LAMBDA);

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
        printf(" Var %s", da_at_end(expr->as.lambda.args, 0));
    }
    printf(") ");
    print_expr(expr->as.lambda.subexpr);
    printf(")");
}

void print_expr(Object *expr) {
    switch (expr->kind) {
    case KIND_NIL:
        printf("Nil");
        break;
    case KIND_BOOL:
        if (BOOL(expr))
            printf("True");
        else
            printf("False");
        break;
    case KIND_INTEGER:
        printf("%lld", INTEGER(expr));
        break;
    case KIND_FLOAT:
        printf("%f", FLOAT(expr));
        break;
    case KIND_STRING:
        printf("%s", STRING(expr));
        break;
    case KIND_SYMBOL:
        printf("%s", SYMBOL(expr));
        break;
    case KIND_LIST:
        print_list(expr);
        break;
    case KIND_BUILTIN:
        break;
    case KIND_LAMBDA:
        print_lambda(expr);
        break;
    }
}
