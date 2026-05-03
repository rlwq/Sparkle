#ifndef PARSER_H
#define PARSER_H

#include <assert.h>
#include <stddef.h>

#include "forwards.h"
#include "lexer.h"

#define PARSER_DONE(p_) ((p_)->tokens_count == 0)
#define PARSER_VALID(p_) (!PARSER_DONE(p_) && !(p_)->is_err)

typedef struct {
    Token *tokens;
    size_t tokens_count;
    LispNodePtrDA exprs;

    GC *gc;

    bool is_err;
} Parser;

Parser *parser_alloc(TokenDA tokens, GC *gc);
void parser_free(Parser *parser);

void parse_current(Parser *parser);
void parse_all(Parser *parser);

LispNodePtrDA extract_exprs(Parser *parser);

#endif
