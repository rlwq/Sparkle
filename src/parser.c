#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynamic_array.h"
#include "lexer.h"
#include "lisp_ast.h"
#include "parser.h"

#define CURR(p_) (assert((p_)), *((p_)->tokens))

Parser* parser_alloc(TokenDA tokens) {
    Parser *parser = malloc(sizeof(Parser));
    assert(parser);

    parser->tokens = tokens.data;
    parser->tokens_count = tokens.size;
    parser->is_err = false;

    da_init(parser->exprs);

    return parser;
}

void parser_free(Parser *parser) {
    da_free(parser->exprs);
    free(parser);
}

bool parser_match(Parser *parser, TokenKind kind) {
    if (!PARSER_VALID(parser))
        return false;

    return CURR(parser).kind == kind;
}

Token parser_advance(Parser *parser) {
    assert(PARSER_VALID(parser));

    Token token = CURR(parser);
    parser->tokens++;
    parser->tokens_count--;
    return token;
}

bool parser_eat(Parser *parser, TokenKind kind) {
    assert(PARSER_VALID(parser));

    if (CURR(parser).kind != kind)
        return false;
    parser_advance(parser);
    return true;
}

bool parser_expect(Parser *parser, TokenKind kind) {
    if (!PARSER_VALID(parser)) {
        parser->is_err = true;
        return false;
    }

    if (CURR(parser).kind != kind) {
        parser->is_err = true;
        return false;
    }

    parser_advance(parser);
    return true;
}

LispAST *parse_expr(Parser *parser) {
    assert(PARSER_VALID(parser));
    
    if (parser_match(parser, TK_EOF)) {
        parser->tokens_count = 0;
        return NULL;
    }

    // S-expr 
    if (parser_eat(parser, TK_L_PAREN)) {
        DA(LispAST *) args;
        da_init(args);

        while (PARSER_VALID(parser) && !parser_match(parser, TK_R_PAREN))
            da_push(args, parse_expr(parser));
        
        if (!parser_expect(parser, TK_R_PAREN)) {
            da_free(args);
            return NULL;
        }

        LispAST *node = gc_alloc(LISP_NIL);
        
        for (size_t i = 0; i < args.size; i++) {
            LispAST *head = gc_alloc(sizeof(LispAST));
            head->kind = LISP_CONS;
            head->as.cons.cdr = node;
            head->as.cons.car = da_at(args, args.size - i - 1);
            node = head;
        }

        da_free(args); 
        return node;
    }
    
    // Integer
    if (parser_match(parser, TK_INTEGER)) {
        LispAST *ast = gc_alloc(sizeof(LispAST));
        ast->kind = LISP_INTEGER;
        ast->as.integer = svtoi(parser_advance(parser).src);
        return ast;
    }
    
    // Symbol
    if (parser_match(parser, TK_SYMBOL)) {
        LispAST *ast = gc_alloc(sizeof(LispAST));
        ast->kind = LISP_SYMBOL;
        ast->as.symbol = parser_advance(parser).src;
        return ast;
    }
    
    // String
    if (parser_match(parser, TK_STRING)) {
        LispAST *ast = gc_alloc(sizeof(LispAST));
        ast->kind = LISP_STRING;
        ast->as.string = sv_shrink(parser_advance(parser).src, 1);
        return ast;
    }
    
    parser->is_err = true;
    return NULL;
}

void parse_current(Parser *parser) {
    assert(PARSER_VALID(parser));
    
    LispAST *expr = parse_expr(parser);
    if (expr) da_push(parser->exprs, expr);
}

void parse_all(Parser *parser) { 
    assert(PARSER_VALID(parser));

    while (PARSER_VALID(parser))
        parse_current(parser);
}
