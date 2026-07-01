#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynamic_array.h"
#include "lexer.h"
#include "string_view.h"

#define CURR(l_) ((l_)->src.size == 0 ? '\0' : *((l_)->src.data))
#define NEXT(l_) ((l_)->src.size < 2 ? '\0' : *((l_)->src.data + 1))
#define BUILD_TOKEN(l_, k_) ((Token){.kind = (k_), .src = sv(NULL, 0), .pos = (l_)->pos})

#define IS_EOF(l_) ((l_)->src.size == 0)

#define issign(c_) ((c_) == '+' || (c_) == '-')
#define in_range(v_, l_, r_) ((v_) >= (l_) && (v_) <= (r_))

#define issymbolic(c_)                                                                             \
    (in_range((c_), '!', '~') && (c_) != '"' && (c_) != '(' && (c_) != ')' && (c_) != '\'')

#define lexer_skip_while(lexer_, expr_)                                                            \
    while ((expr_)) {                                                                              \
        lexer_advance((lexer_));                                                                   \
    }

const char *token_kind_names[] = {
#define X(t_) #t_,
    X_TOKEN_KINDS
#undef X
};

Lexer *lexer_alloc(StringView src, TokenDA *tokens) {
    Lexer *lexer = malloc(sizeof(Lexer));
    assert(lexer);

    lexer->tokens = tokens;

    lexer->src = src;
    lexer->pos = TEXT_BEGIN;

    lexer->marker = src;
    lexer->marker_pos = TEXT_BEGIN;

    lexer->is_err = false;

    return lexer;
}

void lexer_free(Lexer *lexer) {
    free(lexer);
}

char lexer_advance(Lexer *lexer) {
    if (IS_EOF(lexer))
        return '\0';

    char curr = CURR(lexer);

    lexer->src = sv_drop(lexer->src, 1);

    lexer->pos.column++;
    if (curr == '\n') {
        lexer->pos.column = 0;
        lexer->pos.line++;
    }
    return curr;
}

void lexer_marker_set(Lexer *lexer) {
    lexer->marker = lexer->src;
    lexer->marker_pos = lexer->pos;
}

Token lexer_marker_cut(Lexer *lexer, TokenKind kind) {
    size_t size = lexer->src.data - lexer->marker.data;
    StringView src = (StringView){.data = lexer->marker.data, .size = size};
    Token result = (Token){.kind = kind, .src = src, .pos = lexer->marker_pos};
    return result;
}

void lexer_skip_ws(Lexer *lexer) {
    lexer_skip_while(lexer, isspace(CURR(lexer)));
}

void lexer_skip_line_comment(Lexer *lexer) {
    if (CURR(lexer) != ';')
        return;
    lexer_advance(lexer);

    lexer_skip_while(lexer, CURR(lexer) != '\n' && CURR(lexer) != '\0');
}

Token lex_char_token(Lexer *lexer, TokenKind kind) {
    StringView src = sv_take(lexer->src, 1);
    Token result = BUILD_TOKEN(lexer, kind);
    lexer_advance(lexer);
    result.src = src;

    return result;
}

Token lex_numeric_token(Lexer *lexer) {
    lexer_marker_set(lexer);

    if (issign(CURR(lexer)))
        lexer_advance(lexer);

    lexer_skip_while(lexer, isdigit(CURR(lexer)));

    bool is_decimal = false;
    if (CURR(lexer) == '.') {
        is_decimal = true;
        lexer_advance(lexer);
        lexer_skip_while(lexer, isdigit(CURR(lexer)));
    }

    return lexer_marker_cut(lexer, is_decimal ? TK_DECIMAL : TK_INTEGER);
}

Token lex_symbol_token(Lexer *lexer) {
    lexer_marker_set(lexer);

    lexer_skip_while(lexer, issymbolic(CURR(lexer)));

    return lexer_marker_cut(lexer, TK_SYMBOL);
}

Token lex_token(Lexer *lexer) {
    char curr = CURR(lexer);
    char next = NEXT(lexer);

    if (curr == '(')
        return lex_char_token(lexer, TK_L_PAREN);

    if (curr == ')')
        return lex_char_token(lexer, TK_R_PAREN);

    if (curr == '\'')
        return lex_char_token(lexer, TK_QUOTE);

    if (isdigit(curr) || (isdigit(next) && issign(curr)))
        return lex_numeric_token(lexer);

    if (issymbolic(curr))
        return lex_symbol_token(lexer);

    lexer->is_err = true;
    return BUILD_TOKEN(lexer, TK_ERR);
}

void lexer_skip_to_token(Lexer *lexer) {
    while (!IS_EOF(lexer)) {
        if (isspace(CURR(lexer)))
            lexer_skip_ws(lexer);
        else if (CURR(lexer) == ';')
            lexer_skip_line_comment(lexer);
        else
            break;
    }
}

void lex_current(Lexer *lexer) {
    lexer_skip_to_token(lexer);
    if (!IS_EOF(lexer))
        da_push(*(lexer->tokens), lex_token(lexer));
}

void lexer_run(Lexer *lexer) {
    while (!IS_EOF(lexer) && !lexer->is_err) {
        lex_current(lexer);
    }
}
