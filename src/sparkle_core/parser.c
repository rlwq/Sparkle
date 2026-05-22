#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dynamic_array.h"
#include "forwards.h"
#include "gc.h"
#include "lexer.h"
#include "object.h"
#include "parser.h"
#include "string_interner.h"
#include "string_view.h"

#define CURR(p_) (assert((p_)), da_at(*((p_)->tokens), (p_)->cursor))

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

bool parser_match(Parser *parser, TokenKind kind) {
    if (!PARSER_VALID(parser))
        return false;

    return CURR(parser).kind == kind;
}

Token parser_advance(Parser *parser) {
    assert(PARSER_VALID(parser));

    Token token = CURR(parser);
    parser->cursor++;
    return token;
}

bool parser_eat(Parser *parser, TokenKind kind) {
    if (PARSER_DONE(parser))
        return false;

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

Object *parse_expr(Parser *parser);

Object *parse_string(Parser *parser) {
    Token token = parser_advance(parser);
    Object *result = gc_alloc_node(parser->gc, KIND_STRING);
    char *string = malloc(token.src.size + 1);
    memcpy(string, token.src.data + 1, token.src.size - 1);
    string[token.src.size - 2] = '\0';
    STRING(result) = string;
    return result;
}

Object *parse_quote(Parser *parser) {
    if (!parser_eat(parser, TK_QUOTE))
        return NULL;

    Object *subexpr = parse_expr(parser);
    if (subexpr == NULL) {
        parser->is_err = true;
        return NULL;
    }

    Object *result = gc_alloc_node(parser->gc, KIND_CONS);
    CAR(result) = gc_alloc_node(parser->gc, KIND_SYMBOL);
    CAR(result)->as.symbol = parser->si->prebuilt._quote;
    CDR(result) = gc_alloc_node(parser->gc, KIND_CONS);
    CAR(CDR(result)) = subexpr;
    CDR(CDR(result)) = gc_alloc_node(parser->gc, KIND_NIL);
    return result;
}

Object *parse_list(Parser *parser) {
    if (!parser_eat(parser, TK_L_PAREN))
        return NULL;

    if (parser_eat(parser, TK_DOT)) {
        Object *result = gc_alloc_node(parser->gc, KIND_CONS);
        CAR(result) = gc_alloc_node(parser->gc, KIND_NIL);
        CDR(result) = parse_expr(parser);
        parser_expect(parser, TK_R_PAREN);
        return result;
    }

    DA(Object *) args;
    da_init(args);

    while (PARSER_VALID(parser) && !parser_match(parser, TK_R_PAREN) &&
           !parser_match(parser, TK_DOT))
        da_push(args, parse_expr(parser));

    if (parser_eat(parser, TK_DOT)) {
        if (parser_eat(parser, TK_R_PAREN)) {
            da_push(args, gc_alloc_node(parser->gc, KIND_NIL));
        } else {
            da_push(args, parse_expr(parser));
            parser_expect(parser, TK_R_PAREN);
        }
    } else if (parser_eat(parser, TK_R_PAREN)) {
        da_push(args, gc_alloc_node(parser->gc, KIND_NIL));
    } else {
        da_free(args);
        parser->is_err = true;
        return NULL;
    }

    Object *node = da_at_end(args, 0);
    for (size_t i = 1; i < args.size; i++) {
        Object *head = gc_alloc_node(parser->gc, KIND_CONS);
        head->as.cons.cdr = node;
        head->as.cons.car = da_at_end(args, i);
        node = head;
    }

    da_free(args);
    return node;
}

Object *parse_integer(Parser *parser) {
    Object *object = gc_alloc_node(parser->gc, KIND_INTEGER);
    INTEGER(object) = svtolli(parser_advance(parser).src);
    return object;
}

Object *parse_float(Parser *parser) {
    Object *object = gc_alloc_node(parser->gc, KIND_FLOAT);
    FLOAT(object) = svtod(parser_advance(parser).src);
    return object;
}

Object *parse_symbol(Parser *parser) {
    Object *object = gc_alloc_node(parser->gc, KIND_SYMBOL);
    StringView symbol = parser_advance(parser).src;

    object->as.symbol = si_getn(parser->si, symbol.data, symbol.size);
    return object;
}

Object *parse_expr(Parser *parser) {
    if (PARSER_DONE(parser))
        return NULL;

    // string
    if (parser_match(parser, TK_STRING))
        return parse_string(parser);

    // quote
    if (parser_match(parser, TK_QUOTE))
        return parse_quote(parser);

    // list
    if (parser_match(parser, TK_L_PAREN))
        return parse_list(parser);

    // Integer
    if (parser_match(parser, TK_INTEGER))
        return parse_integer(parser);

    // Float
    if (parser_match(parser, TK_DECIMAL))
        return parse_float(parser);

    // Symbol
    if (parser_match(parser, TK_SYMBOL))
        return parse_symbol(parser);

    parser->is_err = true;
    return NULL;
}

void parse_current(Parser *parser) {
    assert(PARSER_VALID(parser));

    Object *expr = parse_expr(parser);
    if (expr)
        da_push(*(parser->exprs), expr);
}

void parser_run(Parser *parser) {
    while (PARSER_VALID(parser))
        parse_current(parser);
}
