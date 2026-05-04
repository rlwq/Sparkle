#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "dynamic_array.h"
#include "forwards.h"
#include "gc.h"
#include "lexer.h"
#include "lisp_node.h"
#include "parser.h"

#define CURR(p_) (assert((p_)), *((p_)->tokens))

Parser *parser_alloc(TokenDA tokens, GC *gc) {
    Parser *parser = malloc(sizeof(Parser));
    assert(parser);

    parser->tokens = tokens.data;
    parser->tokens_count = tokens.size;
    parser->is_err = false;
    parser->gc = gc;

    da_init(parser->exprs);

    return parser;
}

void parser_free(Parser *parser) { free(parser); }

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

LispNode *parse_expr(Parser *parser) {
    assert(PARSER_VALID(parser));

    if (parser_match(parser, TK_EOF)) {
        parser->tokens_count = 0;
        return NULL;
    }

    if (parser_eat(parser, TK_QUOTE)) {
        LispNode *subexpr = parse_expr(parser);
        LispNode *result = gc_alloc_node(parser->gc, LISP_CONS);
        CAR(result) = gc_alloc_node(parser->gc, LISP_SYMBOL);
        CAR(result)->as.symbol = sv_mk("quote"); // TODO: hardcoded value
        CDR(result) = gc_alloc_node(parser->gc, LISP_CONS);
        CAR(CDR(result)) = subexpr;
        CDR(CDR(result)) = gc_alloc_node(parser->gc, LISP_NIL);
        return result;
    }

    // S-expr
    if (parser_eat(parser, TK_L_PAREN)) {
        if (parser_eat(parser, TK_DOT)) {
            LispNode *result = gc_alloc_node(parser->gc, LISP_CONS);
            CAR(result) = gc_alloc_node(parser->gc, LISP_NIL);
            CDR(result) = parse_expr(parser);
            parser_expect(parser, TK_R_PAREN);
            return result;
        }

        DA(LispNode *) args;
        da_init(args);

        while (PARSER_VALID(parser) && !parser_match(parser, TK_R_PAREN) &&
               !parser_match(parser, TK_DOT))
            da_push(args, parse_expr(parser));

        if (parser_eat(parser, TK_DOT)) {
            if (parser_eat(parser, TK_R_PAREN)) {
                da_push(args, gc_alloc_node(parser->gc, LISP_NIL));
            } else {
                da_push(args, parse_expr(parser));
                parser_expect(parser, TK_R_PAREN);
            }
        } else if (parser_eat(parser, TK_R_PAREN)) {
            da_push(args, gc_alloc_node(parser->gc, LISP_NIL));
        } else {
            da_free(args);
            parser->is_err = true;
            return NULL;
        }

        LispNode *node = da_at_end(args, 0);
        for (size_t i = 1; i < args.size; i++) {
            LispNode *head = gc_alloc_node(parser->gc, LISP_CONS);
            head->as.cons.cdr = node;
            head->as.cons.car = da_at_end(args, i);
            node = head;
        }

        da_free(args);
        return node;
    }

    // Integer
    if (parser_match(parser, TK_INTEGER)) {
        LispNode *ast = gc_alloc_node(parser->gc, LISP_INTEGER);
        ast->as.integer = svtoi(parser_advance(parser).src);
        return ast;
    }

    // Symbol
    if (parser_match(parser, TK_SYMBOL)) {
        LispNode *ast = gc_alloc_node(parser->gc, LISP_SYMBOL);
        ast->as.symbol = parser_advance(parser).src;
        return ast;
    }

    // String
    if (parser_match(parser, TK_STRING)) {
        LispNode *ast = gc_alloc_node(parser->gc, LISP_STRING);
        ast->as.string = sv_shrink(parser_advance(parser).src, 1);
        return ast;
    }

    parser->is_err = true;
    return NULL;
}

void parse_current(Parser *parser) {
    assert(PARSER_VALID(parser));

    LispNode *expr = parse_expr(parser);
    if (expr)
        da_push(parser->exprs, expr);
}

void parse_all(Parser *parser) {
    assert(PARSER_VALID(parser));

    while (PARSER_VALID(parser))
        parse_current(parser);
}

LispNodePtrDA extract_exprs(Parser *parser) {
    LispNodePtrDA result = parser->exprs;
    da_nullify(parser->exprs);
    return result;
}
