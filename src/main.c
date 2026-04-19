#include "string_view.h"
#include "tokenizer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    LISP_NIL,
    LISP_CONS,
    LISP_SYMBOL,
    LISP_INTEGER,
    LISP_STRING,
} LISP_AST_KIND;

typedef struct LispAST LispAST;

typedef struct {
    LispAST *car;
    LispAST *cdr;
} Cons;

struct LispAST {
    LISP_AST_KIND kind;
    union {
        StringView symbol;
        StringView string;
        Cons cons;
        int integer;
    } as;
};

int svtoi(StringView sv) {
    int result = 0;
    
    for (size_t i = 0; i < sv.size; i++)
        result = 10 * result + sv_at(sv, i) - '0';

    return result;
}

LispAST *parse_expr(Token *tokens, size_t *curr, size_t size) {
    // S-expr
    assert(*curr < size);
    if (tokens[*curr].kind == TK_L_PAREN) {
        (*curr)++;
        
        // TODO: rewrite that
        LispAST **subexprs = malloc(sizeof(LispAST *) * 256);
        size_t subexprs_count = 0;

        while (tokens[*curr].kind != TK_R_PAREN)
            subexprs[subexprs_count++] = parse_expr(tokens, curr, size);
        
        LispAST *result = malloc(sizeof(LispAST));
        result->kind = LISP_NIL;
        
        for (size_t i = 0; i < subexprs_count; i++) {
            LispAST *head = malloc(sizeof(LispAST));
            head->kind = LISP_CONS;
            head->as.cons.cdr = result;
            head->as.cons.car = subexprs[subexprs_count - i - 1];
            result = head;
        }
        free(subexprs);
        
        assert(tokens[*curr].kind == TK_R_PAREN);
        (*curr)++;
        
        return result;
    }
    
    // Integer
    if (tokens[*curr].kind == TK_INTEGER) {
        LispAST *ast = malloc(sizeof(LispAST));
        ast->kind = LISP_INTEGER;
        ast->as.integer = svtoi(tokens[*curr].src);
        
        (*curr)++;
        return ast;
    }
    
    // Symbol
    if (tokens[*curr].kind == TK_SYMBOL) {
        LispAST *ast = malloc(sizeof(LispAST));
        ast->kind = LISP_SYMBOL;
        ast->as.symbol = tokens[*curr].src;
        
        (*curr)++;
        return ast;
    }
    
    // String
    if (tokens[*curr].kind == TK_STRING) {
        LispAST *ast = malloc(sizeof(LispAST));
        ast->kind = LISP_STRING;
        ast->as.string = sv_shrink(tokens[*curr].src, 1);
        
        (*curr)++;
        return ast;
    }

    assert(0 && "Unreachable");
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

        size_t cursor = 0;
        LispAST *result = parse_expr(t.tokens.data, &cursor, t.tokens.size);
        LispAST *r2 = eval(result);
        print_expr(result);
        printf("\n");
        print_expr(r2);
        printf("\n");
    }
    return 0;
}

