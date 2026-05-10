#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynamic_array.h"
#include "gc.h"
#include "lexer.h"
#include "parser.h"
#include "special_forms.h"
#include "string_interner.h"
#include "string_view.h"
#include "vm.h"

#include "builtins.h"

#define RED "\033[31m"
#define RESET "\033[0m"

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

    StringInterner *si = si_alloc();

    // TODO: Refactor this. Needs some kind of special form registry
    for (size_t i = 0; i < SPECIAL_FORMS_COUNT; i++)
        SPECIAL_FORMS[i].keyword = si_get(si, SPECIAL_FORMS[i].keyword);

    char *src = read_file(argv[1]);
    StringView prog = sv_mk(src);

    Lexer *lexer = lexer_alloc(prog);
    lex_all(lexer);

    if (lexer->is_err) {
        printf(RED "%s:%zu:%zu: [PARSE ERROR] Unexpected character: " SV_FMT "\n" RESET, argv[1],
               lexer->line + 1, lexer->column + 1,
               SV_ARGS(sv_take(lexer->src, sv_find(lexer->src, '\n'))));

        free(src);
        lexer_free(lexer);
        si_free(si);
        return 1;
    }

    TokenDA tokens = extract_tokens(lexer);

    GC *gc = gc_alloc();
    Parser *parser = parser_alloc(tokens, gc, si);
    parse_all(parser);
    LispNodePtrDA exprs = extract_exprs(parser);

    if (parser->is_err) {
        printf(RED "%s:%zu:%zu: [PARSE ERROR] Unexpected token \"" SV_FMT "\".\n" RESET, argv[1],
               parser->tokens->line + 1, parser->tokens->column + 1, SV_ARGS(parser->tokens->src));

        free(src);
        lexer_free(lexer);
        parser_free(parser);
        da_free(tokens);
        da_free(exprs);
        gc_free(gc);
        si_free(si);
        return 1;
    }

    VM *vm = vm_alloc(exprs, gc, si);

    vm_push_scope(vm, gc_alloc_scope(gc, NULL));

    register_builtins(vm);

    if (!VM_DONE(vm))
        vm_eval_all(vm);

    bool in_err = vm->is_err;
    ExceptionKind exception;
    if (in_err)
        exception = vm->exception;

    vm_free(vm);
    da_free(exprs);

    parser_free(parser);
    da_free(tokens);

    gc_free(gc);

    lexer_free(lexer);
    free(src);

    si_free(si);

    if (!in_err)
        return 0;

    printf(RED "[RUNTIME ERROR] ");
    switch (exception) {
    case INVALID_SPECIAL_FORM:
        printf("Invalid special form.");
        break;
    case SYMBOL_REBINDING:
        printf("Symbol is already bound.");
        break;
    case SYMBOL_UNDEFINED:
        printf("Symbol has no definition.");
        break;
    case UNCALLABLE_CALL:
        printf("Can't use this kind of object as a function.");
        break;
    case WRONG_ARITY:
        printf("Wrong arity for a function call.");
        break;
    case WRONG_TYPE:
        printf("Function expected some other object type.");
        break;
    case WRONG_VALUE:
        printf("Function expected some other value.");
        break;
    }
    printf("\n" RESET);
    return 1;
}
