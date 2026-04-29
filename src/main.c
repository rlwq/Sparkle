#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "gc.h"
#include "dynamic_array.h"
#include "string_view.h"
#include "lexer.h"
#include "parser.h"
#include "lisp_node.h"
#include "vm.h"
#include "debug.h"

void lisp_int_eq(size_t args_count, VM *vm) {
    assert(args_count == 2);
    LispNode *value2 = vm_pop_value(vm);
    LispNode *value1 = vm_pop_value(vm);

    if (value1->as.integer == value2->as.integer) {
        LispNode *node = gc_alloc_node(vm->gc, LISP_INTEGER);
        node->as.integer = 1;

        vm_push_value(vm, node);
        return;
    }

    vm_push_value(vm, gc_alloc_node(vm->gc, LISP_NIL));
}

void lisp_sub(size_t args_count, VM *vm) {
    assert(args_count == 2);
    LispNode *value2 = vm_pop_value(vm);
    LispNode *value1 = vm_pop_value(vm);
    
    LispNode *node = gc_alloc_node(vm->gc, LISP_INTEGER);
    node->as.integer = value1->as.integer - value2->as.integer;
    
    vm_push_value(vm, node);
}

void lisp_add(size_t args_count, VM *vm) {
    int result_value = 0;

    for (size_t i = 0; i < args_count; i++) {
        LispNode *popped = vm_pop_value(vm);
        result_value += popped->as.integer;
    }
    
    LispNode *result = gc_alloc_node(vm->gc, LISP_INTEGER);
    result->as.integer = result_value;
    vm_push_value(vm, result);
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
    
    GC *gc = gc_alloc();
    Parser *parser = parser_alloc(tokens, gc);
    parse_all(parser);

    if (parser->is_err) {
        printf("%s:%zu:%zu: [ERROR] Unexpected token \""SV_FMT"\".\n",
                argv[1], parser->tokens->line + 1, parser->tokens->column + 1,
                SV_ARGS(parser->tokens->src));
        
        free(src);
        lexer_free(lexer);
        return 1;
    }
    
    LispNodePtrDA exprs = extract_exprs(parser);

    VM *vm = vm_alloc(exprs, gc);

    vm_push_scope(vm, gc_alloc_scope(gc, NULL));
    //
    vm_register_builtin(vm, sv_mk("+"), lisp_add);
    vm_register_builtin(vm, sv_mk("-"), lisp_sub);
    vm_register_builtin(vm, sv_mk("="), lisp_int_eq);
    eval_all(vm);

    //TODO: maybe should be an iterative approach

    LispNodePtrDA results = extract_results(vm);

    for (size_t i = 0; i < results.size; i++) {
        print_expr(da_at(results, i));
        printf("\n");
    }
    //
    // for (Scope *scope = gc->scopes_heap; scope != NULL; scope = scope->heap_next) {
    //     printf("SCOPE %zu\n", (unsigned long) scope);
    //     printf("PAREN %zu\n", (unsigned long) scope->parent);
    //     for (size_t i = 0; i < scope->symbols.size; i++) {
    //         printf(SV_FMT" = ", SV_ARGS(da_at(scope->symbols, i)));
    //         print_expr(da_at(scope->values, i));
    //         printf("\n");
    //     }
    //     printf("===========\n");
    // }
    
    vm_free(vm);
    da_free(exprs);
    da_free(results);

    parser_free(parser);
    da_free(tokens);
    
    gc_free(gc);

    lexer_free(lexer);
    free(src);

    return 0;
}

