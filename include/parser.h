#ifndef PARSER_H
#define PARSER_H

#include "forwards.h"
#include "lexer.h"
#include "string_interner.h"

#include <assert.h>
#include <stddef.h>

#define PARSER_DONE(p_) ((p_)->cursor == (p_)->tokens->size)
#define PARSER_VALID(p_) (!PARSER_DONE(p_) && !(p_)->is_err)

typedef struct {
    TokenDA *tokens;
    size_t cursor;

    ObjectPtrDA *exprs;

    GC *gc;
    StringInterner *si;

    bool is_err;
} Parser;

Parser *parser_alloc(TokenDA *tokens, ObjectPtrDA *exprs, GC *gc, StringInterner *si);
void parser_free(Parser *parser);

void parser_run(Parser *parser);

#endif
