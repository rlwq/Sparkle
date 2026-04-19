#include "dynamic_array.h"
#include "tokenizer.h"
#include "lisp_ast.h"
#include "parser.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

Parser parser_init(TokenDA tokens) {
    Parser result;
    result.tokens = tokens;
    result.cursor = 0;
    return result;
}

Token parser_lookup(Parser *parser) {
    assert(PARSER_VALID_STATE(*parser));
    return da_at(parser->tokens, parser->cursor);
}

bool parser_match(Parser *parser, TokenKind kind) {
    assert(PARSER_VALID_STATE(*parser));
    return parser_lookup(parser).kind == kind;
}

Token parser_advance(Parser *parser) {
    assert(PARSER_VALID_STATE(*parser));
    Token result = parser_lookup(parser);
    parser->cursor++;
    return result;
}

bool parser_eat(Parser *parser, TokenKind kind) {
    assert(PARSER_VALID_STATE(*parser));
    if (parser_lookup(parser).kind != kind)
        return false;
    parser_advance(parser);
    return true;
}

/*
 * <expr> = <int_lit> | <str_lit> | <NIL_lit> | <sexpr>
 * <sexpr> + '(' {<expr>} ')'
 * */

LispAST *parse_expr(Parser *parser) {
    assert(PARSER_VALID_STATE(*parser));
    
    // S-expr 
    if (parser_eat(parser, TK_L_PAREN)) {
        DA(LispAST *) args;
        da_init(args);

        while (!parser_eat(parser, TK_R_PAREN))
            da_push(args, parse_expr(parser));
        
        LispAST *result = malloc(sizeof(LispAST));
        result->kind = LISP_NIL;
        
        for (size_t i = 0; i < args.size; i++) {
            LispAST *head = malloc(sizeof(LispAST));
            head->kind = LISP_CONS;
            head->as.cons.cdr = result;
            head->as.cons.car = da_at(args, args.size - i - 1);
            result = head;
        }

        da_free(args);
        
        return result;
    }
    
    // Integer
    if (parser_match(parser, TK_INTEGER)) {
        LispAST *ast = malloc(sizeof(LispAST));
        ast->kind = LISP_INTEGER;
        ast->as.integer = svtoi(parser_advance(parser).src);
        return ast;
    }
    
    // Symbol
    if (parser_match(parser, TK_SYMBOL)) {
        LispAST *ast = malloc(sizeof(LispAST));
        ast->kind = LISP_SYMBOL;
        ast->as.symbol = parser_advance(parser).src;
        return ast;
    }
    
    // String
    if (parser_match(parser, TK_STRING)) {
        LispAST *ast = malloc(sizeof(LispAST));
        ast->kind = LISP_STRING;
        ast->as.string = sv_shrink(parser_advance(parser).src, 1);
        return ast;
    }

    assert(0 && "Unreachable");
}
