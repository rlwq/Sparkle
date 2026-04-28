#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

#include "lexer.h"
#include "dynamic_array.h"
#include "string_view.h"

#define CURR(l_) (sv_head((l_)->src))
#define VALID(l_) (!(l_)->is_err && !(l_)->is_eof)
#define EMMIT_TOKEN(l_, k_) ((Token) { .kind = (k_), .src = sv(NULL, 0), .line = (l_)->line, .column = (l_)->column })

Lexer *lexer_alloc(StringView src) {
    Lexer *lexer = malloc(sizeof(Lexer));
    assert(lexer);

    lexer->src = src;
    da_init(lexer->tokens);

    lexer->line = 0;
    lexer->column = 0;

    lexer->is_eof = false;
    lexer->is_err = false;

    return lexer;
}

void lexer_free(Lexer *lexer) {
    free(lexer);
}

char lexer_advance(Lexer *lexer) {
    if (!VALID(lexer)) return '\0';

    char curr = sv_head(lexer->src);
    lexer->src = sv_drop(lexer->src, 1);
    
    lexer->column++;
    if (curr == '\n') {
        lexer->column = 0;
        lexer->line++;
    }
    
    if (!lexer->src.size)
        lexer->is_eof = true;

    return curr;
}

Token lex_char_token(Lexer *lexer, TokenKind kind) {
    assert(VALID(lexer));

    StringView src = sv_take(lexer->src, 1);
    lexer_advance(lexer);
    Token result = EMMIT_TOKEN(lexer, kind);
    result.src = src;

    return result;
}


bool issymbolchar(char c) {
    return (unsigned char) c <= 127 && !isspace(c) && c != '(' && c != ')' && c != '"';
}

void lexer_skip_ws(Lexer *lexer) {
    while(VALID(lexer) && isspace(sv_head(lexer->src)))
        lexer_advance(lexer);
}

Token lex_integer(Lexer *lexer) {
    assert(VALID(lexer));

    Token result = EMMIT_TOKEN(lexer, TK_INTEGER); 
    
    StringView src = lexer->src;
    size_t size = 0;
    if (CURR(lexer) == '+' || CURR(lexer) == '-') {
        size++;
        lexer_advance(lexer);
    }

    while (VALID(lexer) && isdigit(CURR(lexer))) {
        size++;
        lexer_advance(lexer);
    }
    
    result.src = sv_take(src, size);
    return result;
}

Token lex_symbol(Lexer *lexer) { 
    assert(VALID(lexer));

    Token result = EMMIT_TOKEN(lexer, TK_SYMBOL); 
     
    StringView src = lexer->src;
    size_t size = 0;
    while (VALID(lexer) && issymbolchar(CURR(lexer))) {
        size++;
        lexer_advance(lexer);
    }
    
    result.src = sv_take(src, size);
    return result;
}

Token lex_token(Lexer *lexer) {
    assert(!lexer->is_err);
    
    lexer_skip_ws(lexer);
    
    if (lexer->is_eof)
        return EMMIT_TOKEN(lexer, TK_EOF);

    char curr = sv_head(lexer->src);
    char next = sv_next(lexer->src);

    if (curr == '(')
        return lex_char_token(lexer, TK_L_PAREN);

    if (curr == ')')
        return lex_char_token(lexer, TK_R_PAREN);

    if (isdigit(curr) || ((curr == '+' || curr == '-') && isdigit(next)))
        return lex_integer(lexer);
    
    if (issymbolchar(curr))
        return lex_symbol(lexer);

    lexer->is_err = true;
    return EMMIT_TOKEN(lexer, TK_ERR);
}

void lex_current(Lexer *lexer) {
    assert(VALID(lexer));
    da_push(lexer->tokens, lex_token(lexer));
}

void lex_all(Lexer *lexer) {
    assert(VALID(lexer));

    while(VALID(lexer))
        lex_current(lexer);
}

TokenDA extract_tokens(Lexer *lexer) {
    TokenDA result = lexer->tokens;
    da_nullify(lexer->tokens);
    return result;
}
