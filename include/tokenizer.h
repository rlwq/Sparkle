#include "string_view.h"
#include "dynamic_array.h"

typedef enum {
    TK_L_PAREN,
    TK_R_PAREN,
    TK_SYMBOL,

    TK_INTEGER,
    TK_STRING,

    TK_EOF
} TokenKind;

typedef struct {
    TokenKind kind;
    StringView src;

    size_t line;
    size_t column;
} Token;

typedef DA(Token) TokenDA;

typedef struct {
    StringView remainder;
    TokenDA tokens;
} Tokenizer;

#define TOKEN_FMT "<%d (%zu:%zu) \"%.*s\">"
#define TOKEN_ARGS(t_) (t_).kind, ((t_).line + 1), ((t_).column + 1), (int) (t_).src.size, (t_).src.data

Tokenizer tokenizer_init(StringView src);

Token parse_token(StringView *src);
void tokenize(Tokenizer *tokenizer);
