#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynamic_array.h"
#include "string_view.h"
#include "lexer.h"
#include "parser.h"
#include "lisp_ast.h"
#include "evaluator.h"
#include "debug.h"

LispAST *lisp_int_eq(LispAST *args) {
    if (CAR(args)->as.integer == CAR(CDR(args))->as.integer) {
        LispAST *node = gc_alloc(LISP_INTEGER);
        node->as.integer = 1;
        return node;
    }
    return gc_alloc(LISP_NIL);
}

LispAST *lisp_sub(LispAST *args) {
    LispAST *node = gc_alloc(LISP_INTEGER);
    node->as.integer = CAR(args)->as.integer - CAR(CDR(args))->as.integer;
    return node;
}

LispAST *lisp_add(LispAST *args) {
    int result_value = 0;

    for (; args->kind != LISP_NIL; args = args->as.cons.cdr)
        result_value += args->as.cons.car->as.integer;

    LispAST *node = gc_alloc(LISP_INTEGER);
    node->as.integer = result_value;
    return node;
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

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("USAGE: %s source.rkl\n", argv[0]);
        return 1;
    }
    
    char *src = read_file(argv[1]);
    StringView prog = sv_mk(src);

    Lexer *lexer = lexer_alloc(prog);
    lex_all(lexer);
    
    if (lexer->is_err) {
        printf("%s:%zu:%zu: [ERROR] Unexpected character: " SV_FMT "\n",
                argv[1], lexer->line + 1, lexer->column + 1,
                SV_ARGS(sv_take(lexer->src, sv_find(lexer->src, '\n'))));
        
        free(src);
        lexer_free(lexer);
        return 1;
    }
  
    TokenDA tokens = extract_tokens(lexer);

    Parser *parser = parser_alloc(tokens);
    parse_all(parser);
    
    if (parser->is_err) {
        printf("%s:%zu:%zu: [ERROR] Unexpected token \""SV_FMT"\".\n",
                argv[1], parser->tokens->line + 1, parser->tokens->column + 1,
                SV_ARGS(parser->tokens->src));
        
        free(src);
        lexer_free(lexer);
        return 1;
    }
    
    LispASTPtrDA exprs = extract_exprs(parser);

    Evaluator *evaluator = evaluator_alloc(exprs);
    register_builtin(evaluator, sv_mk("+"), lisp_add);
    register_builtin(evaluator, sv_mk("-"), lisp_sub);
    register_builtin(evaluator, sv_mk("="), lisp_int_eq);
    eval_all(evaluator);

    //TODO: maybe should be an iterative approach

    LispASTPtrDA results = extract_results(evaluator);

    for (size_t i = 0; i < results.size; i++) {
        print_expr(da_at(results, i));
        printf("\n");
    }

    gc_sweep();

    evaluator_free(evaluator);
    da_free(exprs);

    parser_free(parser);
    da_free(tokens);
    
    lexer_free(lexer);
    free(src);

    return 0;
}

