#include "io.h"
#include "forwards.h"
#include "object.h"
#include <assert.h>
#include <stdio.h>

void print_cons(Object *expr) {
    assert(expr->kind == KIND_CONS);

    printf("(");
    for (; CDR(expr)->kind == KIND_CONS; expr = CDR(expr)) {
        print_expr(CAR(expr));
        printf(" ");
    }
    print_expr(CAR(expr));
    expr = CDR(expr);
    if (expr->kind != KIND_NIL) {
        printf(" . ");
        print_expr(expr);
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
        printf(" . %s", da_at_end(expr->as.lambda.args, 0));
    }
    printf(") ");
    print_expr(expr->as.lambda.subexpr);
    printf(")");
}

void print_exception(Object *expr) {
    assert(expr->kind == KIND_EXCEPTION);
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
    case WRONG_VALUE:
        printf("WRONG_VALUE");
        break;
    }
    printf(">");
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
        printf("NOT IMPLEMENTED");
        break;
    case KIND_SYMBOL:
        printf("%s", SYMBOL(expr));
        break;
    case KIND_CONS:
        print_cons(expr);
        break;
    case KIND_BUILTIN:
        break;
    case KIND_LAMBDA:
        print_lambda(expr);
        break;
    case KIND_EXCEPTION:
        print_exception(expr);
        break;
    }
}
