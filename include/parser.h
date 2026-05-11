#ifndef PARSER_H
#define PARSER_H

#include <assert.h>
#include <stddef.h>

#include "forwards.h"
#include "lexer.h"
#include "string_interner.h"

#define PARSER_DONE(p_) ((p_)->tokens_count == 0)
#define PARSER_VALID(p_) (!PARSER_DONE(p_) && !(p_)->is_err)

typedef struct {
    Token *tokens;
    size_t tokens_count;
    ObjectPtrDA exprs;

    GC *gc;
    StringInterner *si;

    bool is_err;
} Parser;

Parser *parser_alloc(TokenDA tokens, GC *gc, StringInterner *si);
void parser_free(Parser *parser);

void parser_run(Parser *parser);

ObjectPtrDA extract_exprs(Parser *parser);

#endif
