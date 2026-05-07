#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynamic_array.h"
#include "forwards.h"
#include "gc.h"
#include "lisp_node.h"
#include "scope.h"
#include "vm.h"
#include "speical_forms.h"

#define CURR(e_) (*((e_)->stmts))

VM *vm_alloc(LispNodePtrDA exprs, GC *gc, StringInterner *si) {
    VM *vm = malloc(sizeof(VM));
    assert(vm);

    vm->stmts = exprs.data;
    vm->stmts_count = exprs.size;

    vm->gc = gc;
    vm->si = si;

    da_init(vm->scope_stack);
    da_init(vm->value_stack);
    da_init(vm->recovery_stack);

    vm->is_err = false;

    return vm;
}

void vm_free(VM *vm) {
    da_free(vm->scope_stack);
    da_free(vm->value_stack);
    da_free(vm->recovery_stack);

    free(vm);
}

void vm_push_recovery(VM *vm, jmp_buf *jmp) {
    RecoveryStackEntry recovery = (RecoveryStackEntry){
        .jmp = jmp, .scopes_count = vm->scope_stack.size, .values_count = vm->value_stack.size};
    da_push(vm->recovery_stack, recovery);
}

void vm_pop_recovery(VM *vm) {
    assert(vm->recovery_stack.size > 0);
    da_pop(vm->recovery_stack);
}

void vm_push_scope(VM *vm, Scope *scope) {
    da_push(vm->scope_stack, scope);
}

void vm_build_scope(VM *vm) {
    vm_push_scope(vm, gc_alloc_scope(vm->gc, VM_CURR_SCOPE(vm)));
}

void vm_pop_scope(VM *vm) {
    assert(vm->scope_stack.size > 0);
    da_pop(vm->scope_stack);
}

void vm_scope_define(VM *vm, StringName name) {
    scope_define(VM_CURR_SCOPE(vm), name, vm_peek(vm));
    vm_pop(vm);
}

// 0 -> 1
void vm_push(VM *vm, LispNode *value) {
    assert(value);
    da_push(vm->value_stack, value);
}

void vm_build_value(VM *vm, LispNodeKind kind) {
    if (gc_check_bounds(vm->gc)) {
        vm_mark(vm);
        gc_sweep(vm->gc);
    }
    vm_push(vm, gc_alloc_node(vm->gc, kind));
}

void vm_build_integer(VM *vm, int value) {
    vm_build_value(vm, LISP_INTEGER);
    vm_peek(vm)->as.integer = value;
}

void vm_build_builtin(VM *vm, LispBuiltin value) {
    vm_build_value(vm, LISP_BUILTIN);
    vm_peek(vm)->as.builtin = value;
}

void vm_build_nil(VM *vm) {
    vm_build_value(vm, LISP_NIL);
}

void vm_build_lambda(VM *vm, LambdaArgs args, bool is_variadic, LispNode *expr, Scope *scope) {
    vm_build_value(vm, LISP_LAMBDA);
    vm_peek(vm)->as.lambda.args = args;
    vm_peek(vm)->as.lambda.is_variadic = is_variadic;
    vm_peek(vm)->as.lambda.subexpr = expr;
    vm_peek(vm)->as.lambda.scope = scope;
}

// 1 -> 0
void vm_pop(VM *vm) {
    ASSERT_HAS(vm, 1);
    assert(vm->value_stack.size > 0);
    da_pop(vm->value_stack);
}

void vm_pop_prev(VM *vm) {
    ASSERT_HAS(vm, 2);
    da_at_end(vm->value_stack, 1) = da_at_end(vm->value_stack, 0);
    da_pop(vm->value_stack);
}

// Node (x), Node (y) -> Node (y), Node (x)
void vm_swap(VM *vm) {
    ASSERT_HAS(vm, 2);
    LispNode *curr = da_at_end(vm->value_stack, 0);
    da_at_end(vm->value_stack, 0) = da_at_end(vm->value_stack, 1);
    da_at_end(vm->value_stack, 1) = curr;
}

// Node (x), Node (y), Node (z) -> Node (y), Node (z), Node (x)
void vm_rot(VM *vm) {
    ASSERT_HAS(vm, 3);
    LispNode *curr = da_at_end(vm->value_stack, 0);
    da_at_end(vm->value_stack, 0) = da_at_end(vm->value_stack, 2);
    da_at_end(vm->value_stack, 2) = da_at_end(vm->value_stack, 1);
    da_at_end(vm->value_stack, 1) = curr;
}

LispNode *vm_peek(VM *vm) {
    assert(vm->value_stack.size);

    return da_at_end(vm->value_stack, 0);
}

void vm_mark(VM *vm) {
    // Marking unevaluated expressions
    for (size_t i = 0; i < vm->stmts_count; i++)
        gc_mark_node(vm->stmts[i]);

    // Marking scopes
    for (size_t i = 0; i < vm->scope_stack.size; i++)
        gc_mark_scope(da_at(vm->scope_stack, i));

    // Marking value stack
    for (size_t i = 0; i < vm->value_stack.size; i++)
        gc_mark_node(da_at(vm->value_stack, i));
}

LispNode *vm_advance(VM *vm) {
    assert(vm->stmts_count > 0);
    LispNode *curr = CURR(vm);

    vm->stmts++;
    vm->stmts_count--;
    return curr;
}

// Cons -> Expr * n
size_t eval_list(VM *vm) {
    assert(vm_peek(vm)->kind == LISP_NIL || vm_peek(vm)->kind == LISP_CONS);

    size_t size = 0;
    while (vm_peek(vm)->kind != LISP_NIL) {
        assert(vm_peek(vm)->kind == LISP_CONS);

        // Evaluate head
        vm_push(vm, CAR(vm_peek(vm)));
        vm_eval_expr(vm);
        vm_swap(vm);

        // Push tail
        vm_push(vm, CDR(vm_peek(vm)));
        vm_pop_prev(vm);
        size++;
    }
    vm_pop(vm);
    return size;
}

void vm_recover(VM *vm, ExcepetionKind exception) {
    assert(vm->recovery_stack.size > 0);

    RecoveryStackEntry recovery = da_at_end(vm->recovery_stack, 0);

    vm->value_stack.size = recovery.values_count;
    vm->scope_stack.size = recovery.scopes_count;
    vm->exception = exception;

    longjmp(*(recovery.jmp), 1);
}

void vm_register_builtin(VM *vm, StringName name, LispBuiltin func_ptr) {
    vm_build_builtin(vm, func_ptr);
    vm_scope_define(vm, name);
}

// Cons (args), Node (lambda) -> Node (result)
void eval_lambda_call(VM *vm) {
    ASSERT_HAS(vm, 2);

    LispNode *lambda = vm_peek(vm);
    vm_swap(vm);

    vm_push_scope(vm, LAMBDA_SCOPE(lambda));
    vm_build_scope(vm);

    for (size_t i = 0; i < LAMBDA_POS_ARGS_N(lambda); i++) {
        if (vm_peek(vm)->kind != LISP_CONS) vm_recover(vm, INVALID_LAMBDA_FORM);
        unpack_cons(vm);
        vm_scope_define(vm, da_at(LAMBDA_ARGS(lambda), i));
    }

    if (!LAMBDA_IS_VARIADIC(lambda) && vm_peek(vm)->kind != LISP_NIL)
        vm_recover(vm, WRONG_ARITY);

    if (LAMBDA_IS_VARIADIC(lambda)) {
        vm_scope_define(vm, da_at_end(LAMBDA_ARGS(lambda), 0));
    }
    else vm_pop(vm);

    vm_push(vm, LAMBDA_SUBEXPR(lambda));
    vm_eval_expr(vm);
    vm_pop_prev(vm);

    vm_pop_scope(vm);
    vm_pop_scope(vm);
}

void vm_eval_all(VM *vm) {
    assert(VM_VALID(vm));

    jmp_buf error_handler;
    vm_push_recovery(vm, &error_handler);

    if (setjmp(error_handler)) {
        vm->is_err = true;
    } else {
        while (VM_VALID(vm)) {
            vm_push(vm, vm_advance(vm));
            vm_eval_expr(vm);
            vm_pop(vm);
            assert(vm->recovery_stack.size == 1);
            assert(vm->scope_stack.size == 1);
            assert(vm->value_stack.size == 0);
        }
    }
    vm_pop_recovery(vm);
}

// Node (Cons) -> Node (Tail), Node (Head)
void unpack_cons(VM *vm) {
    ASSERT_HAS(vm, 1);
    assert(vm_peek(vm)->kind == LISP_CONS);

    vm_push(vm, CDR(vm_peek(vm)));
    vm_swap(vm);
    vm_push(vm, CAR(vm_peek(vm)));
    vm_swap(vm);
    vm_pop(vm);
}

// Node (cons) -> Node * n
size_t unpack_list(VM *vm) {
    assert(vm_peek(vm)->kind == LISP_NIL || vm_peek(vm)->kind == LISP_CONS);

    size_t size = 0;
    while (vm_peek(vm)->kind != LISP_NIL) {
        assert(vm_peek(vm)->kind == LISP_CONS);
        unpack_cons(vm);
        vm_swap(vm);

        size++;
    }

    vm_pop(vm);
    return size;
}

// Node (cons) -> Node (cons)
size_t eval_list_inplace(VM *vm) {
    ASSERT_HAS(vm, 1);

    size_t length = 0;
    while (vm_peek(vm)->kind == LISP_CONS) {
        unpack_cons(vm);
        vm_eval_expr(vm);
        vm_swap(vm);
        length++;
    }

    assert(vm_peek(vm)->kind == LISP_NIL);

    for (size_t i = 0; i < length; i++) {
        vm_build_value(vm, LISP_CONS);
        CDR(vm_peek(vm)) = da_at_end(vm->value_stack, 1);
        CAR(vm_peek(vm)) = da_at_end(vm->value_stack, 2);
        vm_pop_prev(vm);
        vm_pop_prev(vm);
    }

    return length;
}

void unpack_list_n(VM *vm, size_t n) {
    ASSERT_HAS(vm, 1);

    for (; n > 0; n--) {
        assert(vm_peek(vm)->kind == LISP_CONS);
        unpack_cons(vm);
        vm_swap(vm);
    }
}

// ConsNode -> Node
void eval_cons(VM *vm) {
    ASSERT_HAS(vm, 1);
    assert(vm_peek(vm)->kind == LISP_CONS);

    unpack_cons(vm);

    bool is_special_form = try_dispatch_special_form(vm);
    if (is_special_form)
        return;

    vm_eval_expr(vm);

    switch (vm_peek(vm)->kind) {
        case LISP_LAMBDA: {
                              LispNode *lambda = vm_peek(vm);
                              vm_swap(vm);
                              eval_list_inplace(vm);

                              vm_push(vm, lambda);
                              eval_lambda_call(vm);
                              break;
                          }

        case LISP_BUILTIN: {
                               LispNode *builtin = vm_peek(vm);
                               vm_swap(vm);
                               size_t args_count = eval_list_inplace(vm);

                               // [CAUTION] Exception source: wrong builtin arity
                               if (!builtin->as.builtin.is_variadic) {
                                   if (args_count != BUILTIN_ARGS_N(builtin))
                                       vm_recover(vm, WRONG_ARITY);
                                   unpack_list(vm);
                               }

                               if (builtin->as.builtin.is_variadic) {
                                   if (args_count < BUILTIN_ARGS_N(builtin))
                                       vm_recover(vm, WRONG_ARITY);
                                   unpack_list_n(vm, BUILTIN_ARGS_N(builtin));
                               }
                               builtin->as.builtin.func(vm);
                               break;
                           }

        case LISP_CONS:
        case LISP_SYMBOL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_NIL:
                           // [CAUTION] Exception source: uncallale object call
                           vm_recover(vm, UNCALLABLE_CALL);
                           break;
    }

    vm_pop_prev(vm);
}

void vm_scope_get(VM *vm, StringName name) {
    LispNode *lookup_result = scope_get(VM_CURR_SCOPE(vm), name);
    if (!lookup_result) {
        // [CAUTION] Exception source: symbol with no definition
        vm_recover(vm, SYMBOL_UNDEFINED);
        return;
    }
    vm_push(vm, lookup_result);
}

// Symbol -> Node
void eval_symbol(VM *vm) {
    ASSERT_HAS(vm, 1);
    assert(vm_peek(vm)->kind == LISP_SYMBOL);

    StringName name = SYMBOL(vm_peek(vm));
    vm_pop(vm);

    if (name == si_get(vm->si, "NIL"))
        vm_build_nil(vm);
    else
        vm_scope_get(vm, name);
}

// Node -> Node
void vm_eval_expr(VM *vm) {
    ASSERT_HAS(vm, 1);

    switch (vm_peek(vm)->kind) {
        case LISP_NIL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_BUILTIN:
        case LISP_LAMBDA:
            // Do nothing
            break;

        case LISP_SYMBOL:
            eval_symbol(vm);
            break;

        case LISP_CONS:
            eval_cons(vm);
            break;
    }
}

