#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

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
    assert(file);

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    char *src = malloc(size + 1);
    assert(src);

    fread(src, 1, size, file);
    src[size] = '\0';

    fclose(file);
    return src;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("USAGE: %s source.rkl\n", argv[0]);
        return 1;
    }

    char *src = read_file(argv[1]);
    StringView prog = sv_mk(src);

    StringInterner *si = si_alloc();
    GC *gc = gc_alloc();

    Lexer *lexer = lexer_alloc(prog);
    TokenDA tokens = da_empty;

    Parser *parser = parser_alloc(gc, si);
    ObjectPtrDA exprs = da_empty;

    VM *vm = vm_alloc(gc, si);

    bool is_err = false;

    // TODO: Refactor this. Needs some kind of special form registry
    for (size_t i = 0; i < SPECIAL_FORMS_COUNT; i++)
        SPECIAL_FORMS[i].keyword = si_get(si, SPECIAL_FORMS[i].keyword);

    lexer_run(lexer);

    if (lexer->is_err) {
        diag_lexer(argv[1], lexer);
        is_err = true;
        goto cleanup;
    }

    tokens = extract_tokens(lexer);

    parser_load(parser, tokens);
    parser_run(parser);

    if (parser->is_err) {
        diag_parser(argv[1], parser);
        is_err = true;
        goto cleanup;
    }

    exprs = extract_exprs(parser);

    vm_push_scope(vm, gc_alloc_scope(vm->gc, NULL));
    vm_load_instructions(vm, exprs);
    register_builtins(vm);
    vm_run(vm);

    if (vm->is_err) {
        diag_vm(argv[1], vm);
        is_err = true;
        goto cleanup;
    }
cleanup:
    vm_free(vm);
    da_free(exprs);

    parser_free(parser);
    da_free(tokens);
    gc_free(gc);
    si_free(si);
    lexer_free(lexer);
    free(src);

    return (int)is_err;
}
