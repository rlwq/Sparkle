#include <stdio.h>
#include "debug.h"
#include "string_view.h"


void print_expr(LispNode *expr) {
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
                StringView curr = da_at(expr->as.lambda.args, i);
                printf(SV_FMT" ", SV_ARGS(curr));
            }
            printf(") ");
            print_expr(expr->as.lambda.expr);
            printf(")");
        break;
    }
}

