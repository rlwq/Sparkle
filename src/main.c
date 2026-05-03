#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynamic_array.h"
#include "gc.h"
#include "lexer.h"
#include "parser.h"
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

    Lexer *lexer = lexer_alloc(prog);
    lex_all(lexer);

    if (lexer->is_err) {
        printf("%s:%zu:%zu: [ERROR] Unexpected character: " SV_FMT "\n", argv[1], lexer->line + 1,
               lexer->column + 1, SV_ARGS(sv_take(lexer->src, sv_find(lexer->src, '\n'))));

        free(src);
        lexer_free(lexer);
        return 1;
    }

    TokenDA tokens = extract_tokens(lexer);

    GC *gc = gc_alloc();
    Parser *parser = parser_alloc(tokens, gc);
    parse_all(parser);

    if (parser->is_err) {
        printf("%s:%zu:%zu: [ERROR] Unexpected token \"" SV_FMT "\".\n", argv[1],
               parser->tokens->line + 1, parser->tokens->column + 1, SV_ARGS(parser->tokens->src));

        free(src);
        lexer_free(lexer);
        return 1;
    }

    LispNodePtrDA exprs = extract_exprs(parser);

    VM *vm = vm_alloc(exprs, gc);

    vm_push_scope(vm, gc_alloc_scope(gc, NULL));

    for (size_t i = 0; i < BUILTINS_COUNT; i++)
        vm_register_builtin(vm, sv_mk(BUILTINS[i].name), BUILTINS[i].func);

    if (VM_VALID(vm))
        vm_eval_all(vm);

    bool in_err = vm->is_err;

    vm_free(vm);
    da_free(exprs);

    parser_free(parser);
    da_free(tokens);

    gc_free(gc);

    lexer_free(lexer);
    free(src);

    if (in_err) {
        printf("[ERROR] Something went wrong...\n");
        return 1;
    }

    return 0;
}
