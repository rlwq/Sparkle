#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
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

#define CURR(p_) (assert((p_)), *((p_)->tokens))

Parser *parser_alloc(GC *gc, StringInterner *si) {
    Parser *parser = malloc(sizeof(Parser));
    assert(parser);

    parser->is_err = false;
    parser->gc = gc;
    parser->si = si;

    parser->is_err = false;

    da_init(parser->exprs);

    return parser;
}

void parser_load(Parser *parser, TokenDA tokens) {
    parser->tokens = tokens.data;
    parser->tokens_count = tokens.size;
}

void parser_free(Parser *parser) {
    da_free(parser->exprs);
    da_nullify(parser->exprs);
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

Object *parse_expr(Parser *parser) {
    assert(PARSER_VALID(parser));

    if (parser_match(parser, TK_EOF)) {
        parser->tokens_count = 0;
        return NULL;
    }

    if (parser_eat(parser, TK_QUOTE)) {
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

    // S-expr
    if (parser_eat(parser, TK_L_PAREN)) {
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

    // Integer
    if (parser_match(parser, TK_INTEGER)) {
        Object *ast = gc_alloc_node(parser->gc, KIND_INTEGER);
        INTEGER(ast) = svtolli(parser_advance(parser).src);
        return ast;
    }

    // Symbol
    if (parser_match(parser, TK_SYMBOL)) {
        Object *ast = gc_alloc_node(parser->gc, KIND_SYMBOL);
        StringView symbol = parser_advance(parser).src;

        ast->as.symbol = si_getn(parser->si, symbol.data, symbol.size);
        return ast;
    }
    //
    // // String
    // if (parser_match(parser, TK_STRING)) {
    //     Object *ast = gc_alloc_node(parser->gc, KIND_STRING);
    //     ast->as.string = sv_shrink(parser_advance(parser).src, 1);
    //     return ast;
    // }
    //
    parser->is_err = true;
    return NULL;
}

void parse_current(Parser *parser) {
    assert(PARSER_VALID(parser));

    Object *expr = parse_expr(parser);
    if (expr)
        da_push(parser->exprs, expr);
}

void parser_run(Parser *parser) {
    while (PARSER_VALID(parser))
        parse_current(parser);
}

ObjectPtrDA extract_exprs(Parser *parser) {
    ObjectPtrDA result = parser->exprs;
    da_nullify(parser->exprs);
    return result;
}
