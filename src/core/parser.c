#include "parser.h"
#include "dynamic_array.h"
#include "forwards.h"
#include "gc.h"
#include "lexer.h"
#include "object.h"
#include "string_interner.h"
#include "string_view.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CURR(p_) (assert((p_)), da_at(*((p_)->tokens), (p_)->cursor))

// The parser allocates objects with the gc_alloc_* constructors and relies on no
// collection occurring during parsing: parsed objects are not yet rooted in the VM
// stacks. gc_alloc_* never collect on their own (see gc.h), so this holds by
// construction.

Parser *parser_alloc(TokenDA *tokens, ObjectPtrDA *exprs, GC *gc, StringInterner *si) {
    Parser *parser = malloc(sizeof(Parser));
    assert(parser);

    parser->gc = gc;
    parser->si = si;

    parser->exprs = exprs;
    parser->cursor = 0;

    parser->is_err = false;
    parser->err_has_pos = false;
    parser->err_pos = TEXT_BEGIN;
    parser->err_where = sv(NULL, 0);
    parser->err_msg = NULL;
    parser->tokens = tokens;

    return parser;
}

void parser_free(Parser *parser) {
    free(parser);
}

// The first failure wins: an inner cause is more specific than the frame that
// discovers it (a list expecting `)`, a quote expecting an expression), so a
// later, vaguer set is dropped.
static void parser_fail_at(Parser *parser, TextPos pos, StringView where, const char *msg) {
    if (parser->is_err)
        return;
    parser->is_err = true;
    parser->err_has_pos = true;
    parser->err_pos = pos;
    parser->err_where = where;
    parser->err_msg = msg;
}

// Fails at the current token, carrying its text so the report can name what was
// found; at end of input there is no token, so only the message is kept.
static void parser_fail(Parser *parser, const char *msg) {
    if (parser->is_err)
        return;
    if (PARSER_DONE(parser)) {
        parser->is_err = true;
        parser->err_msg = msg;
    } else {
        parser_fail_at(parser, CURR(parser).pos, CURR(parser).src, msg);
    }
}

static bool parser_match(Parser *parser, TokenKind kind) {
    if (!PARSER_VALID(parser))
        return false;

    return CURR(parser).kind == kind;
}

static Token parser_advance(Parser *parser) {
    assert(PARSER_VALID(parser));

    Token token = CURR(parser);
    parser->cursor++;
    return token;
}

static bool parser_eat(Parser *parser, TokenKind kind) {
    if (PARSER_DONE(parser))
        return false;

    if (CURR(parser).kind != kind)
        return false;
    parser_advance(parser);
    return true;
}

static Object *parser_read_expr(Parser *parser);

static unsigned parser_hex_digit(char c) {
    return isdigit(c) ? (unsigned)(c - '0') : (unsigned)(tolower(c) - 'a' + 10);
}

static Object *parser_read_string(Parser *parser) {
    // Held whole: the cursor moves past it here, so its position is no longer at
    // the cursor by the time an escape inside it fails.
    Token token = parser_advance(parser);
    StringView body = sv_drop_end(sv_drop(token.src, 1), 1);

    char *data = malloc(body.size + 1);
    assert(data);
    size_t size = 0;

    for (size_t i = 0; i < body.size; i++) {
        char c = sv_at(body, i);
        if (c != '\\') {
            data[size++] = c;
            continue;
        }

        char escape = sv_at(body, ++i);
        switch (escape) {
        case '\\':
        case '"':
            data[size++] = escape;
            break;
        case 'n':
            data[size++] = '\n';
            break;
        case 'r':
            data[size++] = '\r';
            break;
        case 't':
            data[size++] = '\t';
            break;
        case 'x':
            if (i + 2 >= body.size || !isxdigit(sv_at(body, i + 1)) ||
                !isxdigit(sv_at(body, i + 2))) {
                free(data);
                parser_fail_at(parser, token.pos, sv(NULL, 0), "Invalid hex escape");
                return NULL;
            }
            data[size++] = (char)(parser_hex_digit(sv_at(body, i + 1)) << 4 |
                                  parser_hex_digit(sv_at(body, i + 2)));
            i += 2;
            break;
        default:
            free(data);
            parser_fail_at(parser, token.pos, sv(NULL, 0), "Invalid string escape");
            return NULL;
        }
    }

    data = realloc(data, size + 1);
    assert(data);

    return gc_alloc_string_own(parser->gc, data, size);
}

static Object *parser_read_quote(Parser *parser) {
    if (!parser_eat(parser, TK_QUOTE))
        return NULL;

    Object *subexpr = parser_read_expr(parser);
    if (subexpr == NULL) {
        parser_fail(parser, "Expected an expression");
        return NULL;
    }

    // Interned on the spot rather than read from a prepared table: the parser
    // is the only thing under core/ that needs a name the language reserves,
    // and it runs once per quote token, before the VM exists.
    Object *symbol = gc_alloc_symbol(parser->gc, si_get(parser->si, "quote"));

    Object *result = gc_alloc_list(parser->gc);
    da_push(OBJ_LIST_ITEMS(result), symbol);
    da_push(OBJ_LIST_ITEMS(result), subexpr);
    return result;
}

static Object *parser_read_list(Parser *parser) {
    if (!parser_eat(parser, TK_L_PAREN))
        return NULL;

    Object *result = gc_alloc_list(parser->gc);

    while (PARSER_VALID(parser) && !parser_match(parser, TK_R_PAREN))
        da_push(OBJ_LIST_ITEMS(result), parser_read_expr(parser));

    // parser_match guards is_err, so a failure already recorded inside the list
    // (a bad escape leaving the cursor on the `)`) reaches parser_fail as the
    // first, more specific cause rather than advancing past a token it must not.
    if (!parser_match(parser, TK_R_PAREN)) {
        parser_fail(parser, "Expected ')'");
        return NULL;
    }
    parser_advance(parser);

    return result;
}

static Object *parser_read_integer(Parser *parser) {
    return gc_alloc_integer(parser->gc, svtolli(parser_advance(parser).src));
}

static Object *parser_read_float(Parser *parser) {
    return gc_alloc_float(parser->gc, svtod(parser_advance(parser).src));
}

static Object *parser_read_symbol(Parser *parser) {
    StringView symbol = parser_advance(parser).src;
    return gc_alloc_symbol(parser->gc, si_getn(parser->si, symbol.data, symbol.size));
}

static Object *parser_read_expr(Parser *parser) {
    if (PARSER_DONE(parser))
        return NULL;

    // string
    if (parser_match(parser, TK_STRING))
        return parser_read_string(parser);

    // quote
    if (parser_match(parser, TK_QUOTE))
        return parser_read_quote(parser);

    // list
    if (parser_match(parser, TK_L_PAREN))
        return parser_read_list(parser);

    // Integer
    if (parser_match(parser, TK_INTEGER))
        return parser_read_integer(parser);

    // Float
    if (parser_match(parser, TK_DECIMAL))
        return parser_read_float(parser);

    // Symbol
    if (parser_match(parser, TK_SYMBOL))
        return parser_read_symbol(parser);

    parser_fail(parser, "Unexpected token");
    return NULL;
}

static void parser_read_current(Parser *parser) {
    assert(PARSER_VALID(parser));

    Object *expr = parser_read_expr(parser);
    if (expr)
        da_push(*(parser->exprs), expr);
}

void parser_run(Parser *parser) {
    while (PARSER_VALID(parser))
        parser_read_current(parser);
}
