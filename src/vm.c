#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynamic_array.h"
#include "forwards.h"
#include "gc.h"
#include "lisp_node.h"
#include "scope.h"
#include "string_view.h"
#include "vm.h"

#define CURR(e_) (*((e_)->stmts))

VM *vm_alloc(LispNodePtrDA exprs, GC *gc) {
    VM *vm = malloc(sizeof(VM));
    assert(vm);

    vm->stmts = exprs.data;
    vm->stmts_count = exprs.size;

    vm->gc = gc;

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

void vm_push_scope(VM *vm, Scope *scope) { da_push(vm->scope_stack, scope); }

void vm_build_scope(VM *vm) { vm_push_scope(vm, gc_alloc_scope(vm->gc, VM_CURR_SCOPE(vm))); }

void vm_pop_scope(VM *vm) { da_pop(vm->scope_stack); }

void vm_scope_define(VM *vm, StringView name) {
    scope_define(VM_CURR_SCOPE(vm), name, vm_peek_value(vm));
    vm_pop_value(vm);
}

// 0 -> 1
void vm_push_value(VM *vm, LispNode *value) {
    assert(value);
    da_push(vm->value_stack, value);
}

void vm_build_value(VM *vm, LispNodeKind kind) {
    if (gc_check_bounds(vm->gc)) {
        vm_mark(vm);
        gc_sweep(vm->gc);
    }
    vm_push_value(vm, gc_alloc_node(vm->gc, kind));
}

void vm_build_integer(VM *vm, int value) {
    vm_build_value(vm, LISP_INTEGER);
    vm_peek_value(vm)->as.integer = value;
}

void vm_build_builtin(VM *vm, LispBuiltin value) {
    vm_build_value(vm, LISP_BUILTIN);
    vm_peek_value(vm)->as.builtin = value;
}

void vm_build_nil(VM *vm) { vm_build_value(vm, LISP_NIL); }

void vm_build_lambda(VM *vm, StringViewDA args, bool is_variadic, LispNode *expr, Scope *scope) {
    vm_build_value(vm, LISP_LAMBDA);
    vm_peek_value(vm)->as.lambda.args = args;
    vm_peek_value(vm)->as.lambda.is_variadic = is_variadic;
    vm_peek_value(vm)->as.lambda.expr = expr;
    vm_peek_value(vm)->as.lambda.scope = scope;
}

// 1 -> 0
void vm_pop_value(VM *vm) {
    assert(vm->value_stack.size > 0);
    da_pop(vm->value_stack);
}

void vm_pop_prev_value(VM *vm) {
    assert(vm->value_stack.size >= 2);
    da_at_end(vm->value_stack, 1) = da_at_end(vm->value_stack, 0);
    da_pop(vm->value_stack);
}

// Node (x), Node (y) -> Node (y), Node (x)
void vm_swap_value(VM *vm) {
    LispNode *curr = da_at_end(vm->value_stack, 0);
    da_at_end(vm->value_stack, 0) = da_at_end(vm->value_stack, 1);
    da_at_end(vm->value_stack, 1) = curr;
}

// Node (x), Node (y), Node (z) -> Node (y), Node (z), Node (x)
void vm_rot_value(VM *vm) {
    assert(vm->value_stack.size >= 3);
    LispNode *curr = da_at_end(vm->value_stack, 0);
    da_at_end(vm->value_stack, 0) = da_at_end(vm->value_stack, 2);
    da_at_end(vm->value_stack, 2) = da_at_end(vm->value_stack, 1);
    da_at_end(vm->value_stack, 1) = curr;
}

LispNode *vm_peek_value(VM *vm) {
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
    assert(vm_peek_value(vm)->kind == LISP_NIL || vm_peek_value(vm)->kind == LISP_CONS);

    size_t size = 0;
    while (vm_peek_value(vm)->kind != LISP_NIL) {
        assert(vm_peek_value(vm)->kind == LISP_CONS);

        // Evaluate head
        vm_push_value(vm, CAR(vm_peek_value(vm)));
        vm_eval_expr(vm);
        vm_swap_value(vm);

        // Push tail
        vm_push_value(vm, CDR(vm_peek_value(vm)));
        vm_pop_prev_value(vm);
        size++;
    }
    vm_pop_value(vm);
    return size;
}

void vm_recover(VM *vm) {
    assert(vm->recovery_stack.size > 0);

    RecoveryStackEntry recovery = da_at_end(vm->recovery_stack, 0);

    vm->value_stack.size = recovery.values_count;
    vm->scope_stack.size = recovery.scopes_count;

    longjmp(*(recovery.jmp), 1);
}

void vm_register_builtin(VM *vm, StringView name, LispBuiltin func_ptr) {
    vm_build_builtin(vm, func_ptr);
    vm_scope_define(vm, name);
}

void vm_eval_all(VM *vm) {
    assert(VM_VALID(vm));

    jmp_buf error_handler;
    vm_push_recovery(vm, &error_handler);

    if (setjmp(error_handler)) {
        vm->is_err = true;
    } else {
        while (VM_VALID(vm)) {
            vm_push_value(vm, vm_advance(vm));
            vm_eval_expr(vm);
            vm_pop_value(vm);
            assert(vm->recovery_stack.size == 1);
            assert(vm->scope_stack.size == 1);
            assert(vm->value_stack.size == 0);
        }
    }
    vm_pop_recovery(vm);
}

// Node (Cons) -> Node (Tail), Node (Head)
void unpack_cons(VM *vm) {
    assert(vm_peek_value(vm)->kind == LISP_CONS);
    vm_push_value(vm, CDR(vm_peek_value(vm)));
    vm_swap_value(vm);
    vm_push_value(vm, CAR(vm_peek_value(vm)));
    vm_swap_value(vm);
    vm_pop_value(vm);
}

// Node (cons) -> Node * n
size_t unpack_list(VM *vm) {
    assert(vm_peek_value(vm)->kind == LISP_NIL || vm_peek_value(vm)->kind == LISP_CONS);

    size_t size = 0;
    while (vm_peek_value(vm)->kind != LISP_NIL) {
        assert(vm_peek_value(vm)->kind == LISP_CONS);
        unpack_cons(vm);
        vm_swap_value(vm);

        size++;
    }

    vm_pop_value(vm);
    return size;
}

// Symbol (name), Node (value) -> Node
void eval_let_form(VM *vm) {
    vm_swap_value(vm);
    assert(vm_peek_value(vm)->kind == LISP_SYMBOL);
    StringView name = vm_peek_value(vm)->as.symbol;
    vm_pop_value(vm);
    vm_eval_expr(vm);

    scope_define(VM_CURR_SCOPE(vm), name, vm_peek_value(vm));
}

// Node (condition), Node (is_true), Node (is_false) -> result
void eval_if_form(VM *vm) {
    vm_rot_value(vm);
    vm_eval_expr(vm);

    bool is_positive = vm_peek_value(vm)->kind != LISP_NIL;
    vm_pop_value(vm);

    if (is_positive)
        vm_pop_value(vm);
    else
        vm_pop_prev_value(vm);

    vm_eval_expr(vm);
}

// Cons (Args list), Node (subexpr) -> Lambda
void eval_lambda_form(VM *vm) {
    vm_swap_value(vm);
    size_t args_count = unpack_list(vm);

    StringViewDA args;
    da_init(args);

    for (size_t i = 0; i < args_count; i++) {
        da_push(args, vm_peek_value(vm)->as.symbol);
        vm_pop_value(vm);
    }

    LispNode *subexpr = vm_peek_value(vm);

    vm_build_lambda(vm, args, false, subexpr, VM_CURR_SCOPE(vm));
    vm_pop_prev_value(vm);
}

// Node -> Node
void eval_try_form(VM *vm) {
    jmp_buf env;
    vm_push_recovery(vm, &env);

    if (setjmp(env)) {
        vm_pop_value(vm);
        vm_build_integer(vm, 1);
    } else {
        vm_push_value(vm, da_at_end(vm->value_stack, 0));
        vm_eval_expr(vm);
        vm_pop_value(vm);
        vm_pop_value(vm);
        vm_build_nil(vm);
    }

    vm_pop_recovery(vm);
}

// Node -> Node
void eval_quote_form(VM *vm) { assert(vm->value_stack.size > 0); }

// Node * n (args), Node (lambda) -> Node (result)
void eval_lambda_call(VM *vm) {
    LispNode *lambda = vm_peek_value(vm);

    vm_push_scope(vm, lambda->as.lambda.scope);
    vm_build_scope(vm);

    for (size_t i = 0; i < lambda->as.lambda.args.size; i++) {
        vm_swap_value(vm);
        vm_scope_define(vm, da_at(lambda->as.lambda.args, i));
    }

    vm_push_value(vm, lambda->as.lambda.expr);
    vm_eval_expr(vm);
    vm_pop_prev_value(vm);

    vm_pop_scope(vm);
    vm_pop_scope(vm);
}

// Node (Args), Node (Head) -> Node (Maybe :3)
bool try_dispatch_special_form(VM *vm) {
    if (vm_peek_value(vm)->kind != LISP_SYMBOL)
        return false;

    SpecialFormHandler handler = NULL;

    if (sv_eq(vm_peek_value(vm)->as.symbol, sv_mk("if")))
        handler = eval_if_form;

    else if (sv_eq(vm_peek_value(vm)->as.symbol, sv_mk("let")))
        handler = eval_let_form;

    else if (sv_eq(vm_peek_value(vm)->as.symbol, sv_mk("lambda")))
        handler = eval_lambda_form;

    else if (sv_eq(vm_peek_value(vm)->as.symbol, sv_mk("quote")))
        handler = eval_quote_form;

    else if (sv_eq(vm_peek_value(vm)->as.symbol, sv_mk("try")))
        handler = eval_try_form;

    if (handler) {
        vm_pop_value(vm);
        unpack_list(vm);
        handler(vm);
        return true;
    }

    return false;
}

// Node (cons) -> Node (cons)
size_t eval_list_inplace(VM *vm) {
    size_t length = 0;
    while (vm_peek_value(vm)->kind == LISP_CONS) {
        unpack_cons(vm);
        vm_eval_expr(vm);
        vm_swap_value(vm);
        length++;
    }

    assert(vm_peek_value(vm)->kind == LISP_NIL);

    for (size_t i = 0; i < length; i++) {
        vm_build_value(vm, LISP_CONS);
        CDR(vm_peek_value(vm)) = da_at_end(vm->value_stack, 1);
        CAR(vm_peek_value(vm)) = da_at_end(vm->value_stack, 2);
        vm_pop_prev_value(vm);
        vm_pop_prev_value(vm);
    }

    return length;
}

void unpack_list_n(VM *vm, size_t n) {
    for (; n > 0; n--) {
        assert(vm_peek_value(vm)->kind == LISP_CONS);
        unpack_cons(vm);
        vm_swap_value(vm);
    }
}

// ConsNode -> Node
void eval_cons(VM *vm) {
    assert(vm_peek_value(vm)->kind == LISP_CONS);

    unpack_cons(vm);

    bool is_special_form = try_dispatch_special_form(vm);
    if (is_special_form)
        return;

    vm_eval_expr(vm);

    switch (vm_peek_value(vm)->kind) {
    case LISP_LAMBDA: {
        LispNode *lambda = vm_peek_value(vm);
        vm_swap_value(vm);
        size_t args_count = eval_list_inplace(vm);

        // [CAUTION] Exception source: wrong lambda arity

        if (!lambda->as.lambda.is_variadic) {
            if (args_count != LAMBDA_ARGS_N(lambda))
                vm_recover(vm);
            unpack_list(vm);
        }

        if (lambda->as.lambda.is_variadic) {
            if (args_count < LAMBDA_ARGS_N(lambda))
                vm_recover(vm);
            unpack_list_n(vm, LAMBDA_ARGS_N(lambda));
        }

        vm_push_value(vm, lambda);
        eval_lambda_call(vm);
        break;
    }

    case LISP_BUILTIN: {
        LispNode *builtin = vm_peek_value(vm);
        vm_swap_value(vm);
        size_t args_count = eval_list_inplace(vm);

        // [CAUTION] Exception source: wrong builtin arity
        if (!builtin->as.builtin.is_variadic) {
            if (args_count != BUILTIN_ARGS_N(builtin))
                vm_recover(vm);
            unpack_list(vm);
        }

        if (builtin->as.builtin.is_variadic) {
            if (args_count < BUILTIN_ARGS_N(builtin))
                vm_recover(vm);
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
        vm_recover(vm);
        break;
    }

    vm_pop_prev_value(vm);
}

void vm_scope_get(VM *vm, StringView name) {
    LispNode *lookup_result = scope_get(VM_CURR_SCOPE(vm), name);
    if (!lookup_result) {
        // [CAUTION] Exception source: symbol with no definition
        vm_recover(vm);
        return;
    }
    vm_push_value(vm, lookup_result);
}

// Symbol -> Node
void eval_symbol(VM *vm) {
    assert(vm_peek_value(vm)->kind == LISP_SYMBOL);

    StringView name = vm_peek_value(vm)->as.symbol;
    vm_pop_value(vm);

    if (sv_eq(name, sv_mk("NIL")))
        vm_build_nil(vm);
    else
        vm_scope_get(vm, name);
}

// Node -> Node
void vm_eval_expr(VM *vm) {
    switch (vm_peek_value(vm)->kind) {
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
