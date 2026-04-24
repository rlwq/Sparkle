#include <assert.h>
#include <ctype.h>
#include "string_view.h"
#include "tokenizer.h"

Tokenizer *tokenizer_alloc(StringView src) {
    Tokenizer *result = malloc(sizeof(Tokenizer));
    assert(result);
    result->remainder = src;
    da_init(result->tokens);

    return result;
}

Token parse_token(StringView *src) {
    *src = sv_drop_ws(*src);
    
    if (src->size == 0)
        return (Token) { .kind = TK_EOF, .src = *src, .line = 0, .column = 0 }; 

    // L_PAREN
    if (sv_head(*src) == '(') {
        Token token = (Token) { .kind = TK_L_PAREN, .src = sv_take(*src, 1), .line = 0, .column = 0 }; 
        *src = sv_drop(*src, 1);
        return token;
    }

    // R_PAREN
    if (sv_head(*src) == ')') {
        Token token = (Token) { .kind = TK_R_PAREN, .src = sv_take(*src, 1), .line = 0, .column = 0 }; 
        *src = sv_drop(*src, 1);
        return token;
    }

    // INTEGER
    if (isdigit(sv_head(*src))) {
        size_t length = 0;
        while (length < src->size && isdigit(sv_at(*src, length)))
            length++;

        Token token = (Token) { .kind = TK_INTEGER, .src = sv_take(*src, length), .line = 0, .column = 0 }; 
        *src = sv_drop(*src, length);
        return token;
    }

    // STRING
    if (sv_head(*src) == '"') {
        // TODO: check if the closing " exists.
        size_t length = 1;
        while (length < src->size && sv_at(*src, length) != '"')
            length++;
        length++; // dangerous. Must check for an error.

        Token token = (Token) { .kind = TK_STRING, .src = sv_take(*src, length), .line = 0, .column = 0 }; 
        *src = sv_drop(*src, length);
        return token;
    }

    // SYMBOL
    if (isalpha(sv_head(*src))) {
        size_t length = 0;
        while (length < src->size && isalpha(sv_at(*src, length)))
            length++;

        Token token = (Token) { .kind = TK_SYMBOL, .src = sv_take(*src, length), .line = 0, .column = 0 }; 
        *src = sv_drop(*src, length);
        return token;
    }
    
    assert(0 && "Unreachable");
    return (Token) { .kind =  TK_EOF };
}

void tokenize(Tokenizer *tokenizer) {
    while (tokenizer->remainder.size) {
        da_push(tokenizer->tokens, parse_token(&tokenizer->remainder));
    }
}
