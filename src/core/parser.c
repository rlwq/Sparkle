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
    parser->tokens = tokens;

    return parser;
}

void parser_free(Parser *parser) {
    free(parser);
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

static bool parser_expect(Parser *parser, TokenKind kind) {
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

static Object *parser_read_expr(Parser *parser);

static unsigned parser_hex_digit(char c) {
    return isdigit(c) ? (unsigned)(c - '0') : (unsigned)(tolower(c) - 'a' + 10);
}

static Object *parser_read_string(Parser *parser) {
    StringView body = sv_drop_end(sv_drop(parser_advance(parser).src, 1), 1);

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
                parser->is_err = true;
                return NULL;
            }
            data[size++] = (char)(parser_hex_digit(sv_at(body, i + 1)) << 4 |
                                  parser_hex_digit(sv_at(body, i + 2)));
            i += 2;
            break;
        default:
            free(data);
            parser->is_err = true;
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
        parser->is_err = true;
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

    if (!parser_expect(parser, TK_R_PAREN))
        return NULL;

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

    parser->is_err = true;
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
