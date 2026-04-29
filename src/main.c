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

void lisp_int_eq(VM *vm, size_t args_count) {
    assert(args_count == 2);
    int value2 = vm_peek_value(vm)->as.integer;
    vm_pop_value(vm);
    int value1 = vm_peek_value(vm)->as.integer;
    vm_pop_value(vm);

    if (value1 == value2 ) {
        LispNode *node = gc_alloc_node(vm->gc, LISP_INTEGER);
        node->as.integer = 1;

        vm_push_value(vm, node);
        return;
    }

    vm_push_value(vm, gc_alloc_node(vm->gc, LISP_NIL));
}

void lisp_sub(VM *vm, size_t args_count) {
    assert(args_count == 2);
    int value2 = vm_peek_value(vm)->as.integer;
    vm_pop_value(vm);
    int value1 = vm_peek_value(vm)->as.integer;
    vm_pop_value(vm);
    
    LispNode *node = gc_alloc_node(vm->gc, LISP_INTEGER);
    node->as.integer = value1 - value2;
    
    vm_push_value(vm, node);
}

void lisp_print(VM *vm, size_t args_count) {
    for (; args_count > 0; args_count--) {
        LispNode *expr = vm_peek_value(vm);
        print_expr(expr);
        vm_pop_value(vm);
        printf("\n");
    }
    vm_push_value(vm, gc_alloc_node(vm->gc, LISP_NIL));
}


void lisp_add(VM *vm, size_t args_count) {
    int result_value = 0;

    for (size_t i = 0; i < args_count; i++) {
        LispNode *popped = vm_peek_value(vm);
        result_value += popped->as.integer;
        vm_pop_value(vm);
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
    vm_register_builtin(vm, sv_mk("+"), lisp_add);
    vm_register_builtin(vm, sv_mk("-"), lisp_sub);
    vm_register_builtin(vm, sv_mk("="), lisp_int_eq);
    vm_register_builtin(vm, sv_mk("print"), lisp_print);
    eval_all(vm);

    vm_free(vm);
    da_free(exprs);

    parser_free(parser);
    da_free(tokens);
    
    gc_free(gc);

    lexer_free(lexer);
    free(src);

    return 0;
}

