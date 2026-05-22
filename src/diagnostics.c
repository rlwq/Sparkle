#include "diagnostics.h"
#include <assert.h>
#include <stdio.h>

#include "utils.h"
#include "vm.h"

#define RED "\033[31m"
#define RESET "\033[0m"

void diag_lexer(const char *path, Lexer *lexer) {
    assert(lexer->is_err);

    fprintf(stderr, RED "%s:%zu:%zu: [PARSE ERROR] Unexpected character: " SV_FMT "\n" RESET, path,
            lexer->pos.line + 1, lexer->pos.column + 1,
            SV_ARGS(sv_take(lexer->src, sv_find(lexer->src, '\n'))));
}

void diag_parser(const char *path, Parser *parser) {
    assert(parser->is_err);

    fprintf(stderr, RED "%s:%zu:%zu: [PARSE ERROR] Unexpected token \"" SV_FMT "\".\n" RESET, path,
            parser->tokens->pos.line + 1, parser->tokens->pos.column + 1,
            SV_ARGS(parser->tokens->src));
}

void diag_vm(const char *path, VM *vm) {
    assert(vm->is_err);

    fprintf(stderr, RED "%s: [RUNTIME ERROR] ", path);
    if (vm->exception == vm->singletons._VALUE_EXCEPTION)
        fprintf(stderr, "Function expected some other value.");
    else if (vm->exception == vm->singletons._REBINDING_EXCEPTION)
        fprintf(stderr, "Symbol is already bound.");
    else if (vm->exception == vm->singletons._TYPE_EXCEPTION)
        fprintf(stderr, "Function expected some other object type.");
    else if (vm->exception == vm->singletons._ARITY_EXCEPTION)
        fprintf(stderr, "Wrong arity for a function call.");
    else if (vm->exception == vm->singletons._UNDEFINED_EXCEPTION)
        fprintf(stderr, "Symbol has no definition.");
    else if (vm->exception == vm->singletons._UNCALLABLE_EXCEPTION)
        fprintf(stderr, "Can't use this kind of object as a function.");
    else
        UNREACHABLE();
    fprintf(stderr, "\n" RESET);
}
