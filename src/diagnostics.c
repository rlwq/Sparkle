#include "diagnostics.h"
#include <assert.h>
#include <stdio.h>

#include "object.h"
#include "vm.h"

#define RED "\033[31m"
#define RESET "\033[0m"

void diag_lexer(const char *path, Lexer *lexer) {
    assert(lexer->is_err);

    fprintf(stderr, RED "%s:%zu:%zu: [PARSE ERROR] Unexpected character: " SV_FMT "\n" RESET, path,
            lexer->line + 1, lexer->column + 1,
            SV_ARGS(sv_take(lexer->src, sv_find(lexer->src, '\n'))));
}

void diag_parser(const char *path, Parser *parser) {
    assert(parser->is_err);

    fprintf(stderr, RED "%s:%zu:%zu: [PARSE ERROR] Unexpected token \"" SV_FMT "\".\n" RESET, path,
            parser->tokens->line + 1, parser->tokens->column + 1, SV_ARGS(parser->tokens->src));
}

void diag_vm(const char *path, VM *vm) {
    assert(vm->is_err);

    fprintf(stderr, RED "%s: [RUNTIME ERROR] ", path);
    switch (vm->exception) {
    case INVALID_SPECIAL_FORM:
        fprintf(stderr, "Invalid special form.");
        break;
    case SYMBOL_REBINDING:
        fprintf(stderr, "Symbol is already bound.");
        break;
    case SYMBOL_UNDEFINED:
        fprintf(stderr, "Symbol has no definition.");
        break;
    case UNCALLABLE_CALL:
        fprintf(stderr, "Can't use this kind of object as a function.");
        break;
    case WRONG_ARITY:
        fprintf(stderr, "Wrong arity for a function call.");
        break;
    case WRONG_TYPE:
        fprintf(stderr, "Function expected some other object type.");
        break;
    case WRONG_VALUE:
        fprintf(stderr, "Function expected some other value.");
        break;
    }
    fprintf(stderr, "\n" RESET);
}
