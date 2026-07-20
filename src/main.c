#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diagnostics.h"
#include "dynamic_array.h"
#include "forwards.h"
#include "gc.h"
#include "lexer.h"
#include "parser.h"
#include "special_forms.h"
#include "string_interner.h"
#include "string_view.h"
#include "vm.h"

#include "builtins.h"

char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "%s: cannot open file: %s\n", path, strerror(errno));
        exit(1);
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "%s: cannot seek file: %s\n", path, strerror(errno));
        exit(1);
    }

    // -1 here would wrap into a huge size_t below.
    long length = ftell(file);
    if (length < 0) {
        fprintf(stderr, "%s: cannot size file: %s\n", path, strerror(errno));
        exit(1);
    }
    rewind(file);

    size_t size = (size_t)length;

    // Not an assert: NDEBUG would drop it in release.
    char *src = malloc(size + 1);
    if (!src) {
        fprintf(stderr, "%s: out of memory\n", path);
        exit(1);
    }

    if (fread(src, 1, size, file) != size) {
        fprintf(stderr, "%s: cannot read file\n", path);
        exit(1);
    }
    src[size] = '\0';

    fclose(file);
    return src;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s source.rkl\n", argv[0]);
        return 1;
    }

    char *src = read_file(argv[1]);
    StringView prog = sv_mk(src);

    StringInterner *si = si_alloc();
    GC *gc = gc_alloc();

    TokenDA tokens;
    da_init(tokens);

    Lexer *lexer = lexer_alloc(prog, &tokens);

    ObjectPtrDA exprs;
    da_init(exprs);
    Parser *parser = parser_alloc(&tokens, &exprs, gc, si);

    VM *vm = vm_alloc(gc, si);

    bool is_err = false;

    special_forms_attach(vm);

    lexer_run(lexer);

    if (lexer->is_err) {
        diag_lexer(argv[1], lexer);
        is_err = true;
        goto cleanup;
    }

    parser_run(parser);

    if (parser->is_err) {
        diag_parser(argv[1], parser);
        is_err = true;
        goto cleanup;
    }

    vm_push_scope(vm, gc_alloc_scope(vm->gc, NULL));
    vm_load_instructions(vm, exprs);
    register_builtins(vm);
    vm_run(vm);

    if (vm->is_err) {
        diag_vm(argv[1], vm);
        is_err = true;
        goto cleanup;
    }

    // Reverse order of allocation.
cleanup:
    vm_free(vm);
    parser_free(parser);
    da_free(exprs);
    lexer_free(lexer);
    da_free(tokens);
    gc_free(gc);
    si_free(si);
    free(src);

    return (int)is_err;
}
