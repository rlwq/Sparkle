#include "lexer.h"
#include "dynamic_array.h"
#include "string_view.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#define CURR(l_) ((l_)->src.size == 0 ? '\0' : *((l_)->src.data))
#define NEXT(l_) ((l_)->src.size < 2 ? '\0' : *((l_)->src.data + 1))
#define BUILD_TOKEN(l_, k_) ((Token){.kind = (k_), .src = sv(NULL, 0), .pos = (l_)->pos})

#define IS_EOF(l_) ((l_)->src.size == 0)

#define issign(c_) ((c_) == '+' || (c_) == '-')
#define in_range(v_, l_, r_) ((v_) >= (l_) && (v_) <= (r_))

// Mirrors symbol-char in Specification.md: ';' is excluded so a comment
// terminates the symbol it touches.
#define issymbolic(c_)                                                                             \
    (in_range((c_), '!', '~') && (c_) != '"' && (c_) != '(' && (c_) != ')' && (c_) != '\'' &&      \
     (c_) != ';')

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

static char lexer_advance(Lexer *lexer) {
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

static void lexer_marker_set(Lexer *lexer) {
    lexer->marker = lexer->src;
    lexer->marker_pos = lexer->pos;
}

static Token lexer_marker_cut(Lexer *lexer, TokenKind kind) {
    size_t size = lexer->src.data - lexer->marker.data;
    StringView src = (StringView){.data = lexer->marker.data, .size = size};
    Token result = (Token){.kind = kind, .src = src, .pos = lexer->marker_pos};
    return result;
}

static void lexer_skip_ws(Lexer *lexer) {
    lexer_skip_while(lexer, isspace(CURR(lexer)));
}

static void lexer_skip_line_comment(Lexer *lexer) {
    if (CURR(lexer) != ';')
        return;
    lexer_advance(lexer);

    lexer_skip_while(lexer, CURR(lexer) != '\n' && CURR(lexer) != '\0');
}

// Block comments /* ... */ are recognized only between tokens (a '/' inside a
// symbol stays symbolic, so division and names like /* survive in strings and
// symbols). Unlike C they nest: every inner /* needs its own */. Hitting EOF
// before the matching */ is a lexing error.
static void lexer_skip_block_comment(Lexer *lexer) {
    if (CURR(lexer) != '/' || NEXT(lexer) != '*')
        return;
    lexer_advance(lexer);
    lexer_advance(lexer);

    size_t depth = 1;
    while (!IS_EOF(lexer) && depth > 0) {
        if (CURR(lexer) == '/' && NEXT(lexer) == '*') {
            depth++;
            lexer_advance(lexer);
            lexer_advance(lexer);
        } else if (CURR(lexer) == '*' && NEXT(lexer) == '/') {
            depth--;
            lexer_advance(lexer);
            lexer_advance(lexer);
        } else {
            lexer_advance(lexer);
        }
    }

    if (depth > 0)
        lexer->is_err = true;
}

static Token lexer_read_char_token(Lexer *lexer, TokenKind kind) {
    StringView src = sv_take(lexer->src, 1);
    Token result = BUILD_TOKEN(lexer, kind);
    lexer_advance(lexer);
    result.src = src;

    return result;
}

// The shape has already been decided by sv_scan_number, so this only has to
// walk over what the scan measured, keeping line and column tracking intact.
static Token lexer_read_numeric_token(Lexer *lexer, SvNumber kind, size_t length) {
    lexer_marker_set(lexer);

    for (size_t i = 0; i < length; i++)
        lexer_advance(lexer);

    return lexer_marker_cut(lexer, kind == SV_NUMBER_FLOAT ? TK_DECIMAL : TK_INTEGER);
}

static Token lexer_read_string_token(Lexer *lexer) {
    lexer_marker_set(lexer);
    lexer_advance(lexer);

    while (!IS_EOF(lexer) && CURR(lexer) != '"') {
        if (lexer_advance(lexer) == '\\')
            lexer_advance(lexer);
    }

    if (IS_EOF(lexer)) {
        lexer->is_err = true;
        return BUILD_TOKEN(lexer, TK_ERR);
    }

    lexer_advance(lexer);
    return lexer_marker_cut(lexer, TK_STRING);
}

static Token lexer_read_symbol_token(Lexer *lexer) {
    lexer_marker_set(lexer);

    lexer_skip_while(lexer, issymbolic(CURR(lexer)));

    return lexer_marker_cut(lexer, TK_SYMBOL);
}

static Token lexer_read_token(Lexer *lexer) {
    char curr = CURR(lexer);

    if (curr == '(')
        return lexer_read_char_token(lexer, TK_L_PAREN);

    if (curr == ')')
        return lexer_read_char_token(lexer, TK_R_PAREN);

    if (curr == '\'')
        return lexer_read_char_token(lexer, TK_QUOTE);

    if (curr == '"')
        return lexer_read_string_token(lexer);

    // Numbers are recognised by the shared scanner rather than by peeking at a
    // character or two: with a leading point and an exponent in the grammar,
    // how far ahead a number reaches is no longer decidable from the front.
    size_t number_length = 0;
    SvNumber number = sv_scan_number(lexer->src, &number_length);
    if (number != SV_NUMBER_NONE)
        return lexer_read_numeric_token(lexer, number, number_length);

    if (issymbolic(curr))
        return lexer_read_symbol_token(lexer);

    lexer->is_err = true;
    return BUILD_TOKEN(lexer, TK_ERR);
}

static void lexer_skip_to_token(Lexer *lexer) {
    while (!IS_EOF(lexer)) {
        if (isspace(CURR(lexer)))
            lexer_skip_ws(lexer);
        else if (CURR(lexer) == ';')
            lexer_skip_line_comment(lexer);
        else if (CURR(lexer) == '/' && NEXT(lexer) == '*')
            lexer_skip_block_comment(lexer);
        else
            break;
    }
}

static void lexer_read_current(Lexer *lexer) {
    lexer_skip_to_token(lexer);
    if (!IS_EOF(lexer))
        da_push(*(lexer->tokens), lexer_read_token(lexer));
}

void lexer_run(Lexer *lexer) {
    while (!IS_EOF(lexer) && !lexer->is_err) {
        lexer_read_current(lexer);
    }
}
