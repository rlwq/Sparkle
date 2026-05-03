#ifndef LEXER_H
#define LEXER_H

#include "string_view.h"

typedef enum {
    TK_L_PAREN,
    TK_R_PAREN,
    TK_SYMBOL,

    TK_INTEGER,
    TK_STRING,

    TK_EOF,
    TK_ERR,
} TokenKind;

typedef struct {
    TokenKind kind;
    StringView src;

    size_t line;
    size_t column;
} Token;

typedef DA(Token) TokenDA;

typedef struct {
    StringView src;
    TokenDA tokens;

    size_t line;
    size_t column;

    bool is_eof;
    bool is_err;
} Lexer;

#define TOKEN_FMT "<%d (%zu:%zu) \"%.*s\">"
#define TOKEN_ARGS(t_)                                                                             \
    (t_).kind, ((t_).line + 1), ((t_).column + 1), (int)(t_).src.size, (t_).src.data

Lexer *lexer_alloc(StringView src);
void lexer_free(Lexer *lexer);

void lex_current(Lexer *lexer);
void lex_all(Lexer *lexer);

TokenDA extract_tokens(Lexer *lexer);

#endif
