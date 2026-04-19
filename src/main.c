#include "string_view.h"
#include "tokenizer.h"
#include "parser.h"
#include "lisp_ast.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

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
        default: assert(0 && "Unreachable"); break;
    }
}


LispAST *eval(LispAST *expr) {
    switch (expr->kind) {
        case LISP_NIL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_SYMBOL:
            return expr;
        break;
        case LISP_CONS: {
            StringView func = expr->as.cons.car->as.symbol;
            LispAST *arg1 = eval(expr->as.cons.cdr->as.cons.car);
            LispAST *arg2 = eval(expr->as.cons.cdr->as.cons.cdr->as.cons.car);

            if (sv_eq(func, sv_mk("add"))) {
                LispAST *result = malloc(sizeof(LispAST));
                result->kind = LISP_INTEGER;
                result->as.integer = arg1->as.integer + arg2->as.integer;
                return result;
            }
            else {
                assert(0 && "Unreachable");
            }
        } break;
        default:
            assert(0 && "Unreachable");
        break;
    } 
}

int main() {
    char buff[2048];

    while (true) {
        if (!fgets(buff, sizeof(buff), stdin)) break;

        StringView prog = sv_mk(buff);
        Tokenizer t = tokenizer_init(prog);
        tokenize(&t);
        
        Parser p = parser_init(t.tokens);

        LispAST *result = parse_expr(&p);
        LispAST *r2 = eval(result);
        print_expr(result);
        printf("\n");
        print_expr(r2);
        printf("\n");
    }
    return 0;
}

