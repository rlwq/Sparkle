#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynamic_array.h"
#include "string_view.h"
#include "tokenizer.h"
#include "parser.h"
#include "lisp_ast.h"
#include "utils.h"
#include "evaluator.h"

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
            NOT_IMPLEMENTED();
        break;
        case LISP_LAMBDA:
            NOT_IMPLEMENTED();
        break;
    }
}

LispAST *lisp_add(LispAST *args) {
    int result_value = 0;

    for (; args->kind != LISP_NIL; args = args->as.cons.cdr)
        result_value += args->as.cons.car->as.integer;

    LispAST *result = malloc(sizeof(LispAST));
    result->kind = LISP_INTEGER;
    result->as.integer = result_value;
    return result;
}

LispAST *lisp_reverse_list(LispAST *args) {
    DA(LispAST *) new_args;
    da_init(new_args);

    for (LispAST *curr_arg = args;
            curr_arg->kind != LISP_NIL;
            curr_arg = curr_arg->as.cons.cdr) {
            da_push(new_args, curr_arg->as.cons.car);
    }

    LispAST *result = malloc(sizeof(LispAST));
    result->kind = LISP_NIL;
    for (size_t i = 0; i < new_args.size; i++) {
        LispAST *curr = malloc(sizeof(LispAST));
        curr->kind = LISP_CONS;
        curr->as.cons.car = da_at(new_args, new_args.size - i - 1);
        curr->as.cons.cdr = result;
        result = curr;
    }

    da_free(new_args);

    return result;
}

LispAST *lisp_id(LispAST *args) {
    return args;
}

int main() {
    char buff[2048];

    Env env = env_init();

    LispAST *add_func = malloc(sizeof(LispAST));
    add_func->kind = LISP_BUILTIN;
    add_func->as.builtin = lisp_add;

    LispAST *reverse_func = malloc(sizeof(LispAST));
    reverse_func->kind = LISP_BUILTIN;
    reverse_func->as.builtin = lisp_reverse_list;

    LispAST *id_func = malloc(sizeof(LispAST));
    id_func->kind = LISP_BUILTIN;
    id_func->as.builtin = lisp_id;

    env_define(&env, sv_mk("add"), add_func);
    env_define(&env, sv_mk("reverse"), reverse_func);
    env_define(&env, sv_mk("id"), id_func);

    while (true) {
        if (!fgets(buff, sizeof(buff), stdin)) break;

        StringView prog = sv_mk(buff);
        Tokenizer t = tokenizer_init(prog);
        tokenize(&t);
        
        Parser p = parser_init(t.tokens);

        LispAST *result = parse_expr(&p);
        LispAST *r2 = lisp_eval(result, &env);
        print_expr(result);
        printf("\n");
        print_expr(r2);
        printf("\n");
    }
    return 0;
}

