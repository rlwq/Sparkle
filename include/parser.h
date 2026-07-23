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
    // Recorded once, at the first failure - an inner cause is more specific than
    // the frame that discovers it. The reporter reads these instead of the
    // cursor, which has moved past the token in the string-escape case.
    // err_where is the offending token's text; empty when there is no position
    // (end of input), which err_has_pos flags.
    bool err_has_pos;
    TextPos err_pos;
    StringView err_where;
    const char *err_msg;
} Parser;

Parser *parser_alloc(TokenDA *tokens, ObjectPtrDA *exprs, GC *gc, StringInterner *si);
void parser_free(Parser *parser);

void parser_run(Parser *parser);

#endif
