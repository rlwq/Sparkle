#ifndef LEXER_H
#define LEXER_H

#include "string_view.h"

typedef struct {
    size_t line;
    size_t column;
} TextPos;

#define TEXT_BEGIN ((TextPos){.line = 0, .column = 0})

#define X_TOKEN_KINDS                                                                              \
    X(L_PAREN)                                                                                     \
    X(R_PAREN)                                                                                     \
    X(DOT)                                                                                         \
    X(QUOTE)                                                                                       \
    X(SYMBOL)                                                                                      \
    X(INTEGER)                                                                                     \
    X(DECIMAL)                                                                                     \
    X(STRING)                                                                                      \
    X(ERR)

extern const char *token_kind_names[];

typedef enum {
#define X(t_) TK_##t_,
    X_TOKEN_KINDS
#undef X
} TokenKind;

typedef struct {
    TokenKind kind;
    StringView src;

    TextPos pos;
} Token;

typedef DA(Token) TokenDA;

typedef struct {
    StringView src;
    TextPos pos;

    StringView marker;
    TextPos marker_pos;

    TokenDA *tokens;

    bool is_err;
} Lexer;

#define TOKEN_FMT "<%s (%zu:%zu) \"%.*s\">"
#define TOKEN_ARGS(t_)                                                                             \
    token_kind_names[(t_).kind], ((t_).pos.line + 1), ((t_).pos.column + 1), (int)(t_).src.size,   \
        (t_).src.data

Lexer *lexer_alloc(StringView src, TokenDA *tokens);
void lexer_free(Lexer *lexer);

void lexer_run(Lexer *lexer);

#endif
