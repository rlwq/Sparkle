#ifndef PARSER_H
#define PARSER_H

#include "tokenizer.h"
#include "lisp_ast.h"
#include <assert.h>
#include <stddef.h>

#define PARSER_VALID_STATE(p_) ((p_).cursor < (p_).tokens.size)

typedef struct {
    TokenDA tokens;
    size_t cursor;
    da_lisp_ast_ptr exprs;
} Parser;

Parser *parser_alloc(TokenDA tokens);
void parser_free(Parser *parser);

Token parser_lookup(Parser *parser); 
bool parser_match(Parser *parser, TokenKind kind);
Token parser_advance(Parser *parser);
bool parser_eat(Parser *parser, TokenKind kind);

LispAST *parse_expr(Parser *parser);
void parse(Parser *parser);

#endif 
