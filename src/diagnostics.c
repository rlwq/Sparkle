#include "diagnostics.h"
#include "vm.h"

#include <assert.h>
#include <stdio.h>

#define RED "\033[31m"
#define RESET "\033[0m"

// Diagnostics go to stderr while the program's own output goes to stdout, and
// the two are buffered differently: stderr is unbuffered, stdout is block
// buffered as soon as it is not a terminal. Without this the report jumps ahead
// of the output it is about whenever either stream is redirected - which is
// exactly what a piped REPL session and the test runner both do.
static void flush_output(void) {
    fflush(stdout);
}

void diag_lexer(const char *path, Lexer *lexer) {
    flush_output();
    assert(lexer->is_err);

    fprintf(stderr, RED "%s:%zu:%zu: [PARSE ERROR] Unexpected character: " SV_FMT "\n" RESET, path,
            lexer->pos.line + 1, lexer->pos.column + 1,
            SV_ARGS(sv_take(lexer->src, sv_find(lexer->src, '\n'))));
}

void diag_parser(const char *path, Parser *parser) {
    flush_output();
    (void)parser;
    assert(parser->is_err);
    fprintf(stderr, RED "%s: [PARSE ERROR] Invalid or incomplete expression.\n" RESET, path);
}

void diag_vm(const char *path, VM *vm) {
    flush_output();
    assert(vm->is_err);

    fprintf(stderr, RED "%s: [RUNTIME ERROR] ", path);

    // Compared by interned name rather than by object: a kind that reached here
    // through throw is a fresh symbol, since capitalized symbols self-evaluate
    // into one, and would match no singleton by identity.
    const char *message = NULL;
#define X(name_, msg_)                                                                             \
    if (OBJ_SYMBOL(vm->exception) == OBJ_SYMBOL(vm->singletons._##name_))                          \
        message = (msg_);
    X_RUNTIME_EXCEPTIONS
#undef X

    // A kind the language does not define is one the program invented, so the
    // name it chose is the whole of what can be said about it.
    if (message)
        fprintf(stderr, "%s", message);
    else
        fprintf(stderr, "%s raised.", OBJ_SYMBOL(vm->exception));

    fprintf(stderr, "\n" RESET);
}
