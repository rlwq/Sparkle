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
    (void)parser;
    assert(parser->is_err);
    fprintf(stderr, RED "%s: [PARSE ERROR] Invalid or incomplete expression.\n" RESET, path);
}

void diag_vm(const char *path, VM *vm) {
    assert(vm->is_err);

    fprintf(stderr, RED "%s: [RUNTIME ERROR] ", path);

    const char *message = NULL;
#define X(name_, msg_)                                                                             \
    if (vm->exception == vm->singletons._##name_)                                                  \
        message = (msg_);
    X_RUNTIME_EXCEPTIONS
#undef X

    if (message)
        fprintf(stderr, "%s", message);
    else
        UNREACHABLE();

    fprintf(stderr, "\n" RESET);
}
