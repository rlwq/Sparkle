#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynamic_array.h"
#include "string_view.h"
#include "tokenizer.h"
#include "parser.h"
#include "lisp_ast.h"
#include "evaluator.h"

LispAST *lisp_add(LispAST *args) {
    int result_value = 0;

    for (; args->kind != LISP_NIL; args = args->as.cons.cdr)
        result_value += args->as.cons.car->as.integer;

    LispAST *result = gc_alloc(LISP_INTEGER);
    result->as.integer = result_value;
    return result;
}

void parse(Parser *parser) {
    assert(PARSER_VALID_STATE(*parser));
    while (da_at(parser->tokens, parser->cursor).kind != TK_EOF) {
        da_push(parser->exprs, parse_expr(parser));
    }
}

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

int main([[maybe_unused]] int argc, char** argv) {
    assert(argc == 2);
    char *src = read_file(argv[1]);
    StringView prog = sv_mk(src);

    Tokenizer *t = tokenizer_alloc(prog);
    tokenize(t);

    Parser *p = parser_alloc(t->tokens);
    parse(p);

    Evaluator *evaluator = evaluator_alloc(p->exprs);
    register_builtin(evaluator, sv_mk("add"), lisp_add);
    evaluate_all(evaluator);
    
    printf("%zu\n", heap_size());
    env_mark(evaluator->global_scope);
    gc_sweep();
    printf("%zu\n", heap_size());
    return 0;
}

