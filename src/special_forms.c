#include "speical_forms.h"
#include "vm.h"
#include "scope.h"

// Node (Args), Node (Head) -> Node (Maybe :3)
bool try_dispatch_special_form(VM *vm) {
    ASSERT_HAS(vm, 2);

    if (vm_peek(vm)->kind != LISP_SYMBOL)
        return false;

    SpecialFormHandler handler = NULL;

    if (sv_eq(SYMBOL(vm_peek(vm)), sv_mk("if")))
        handler = eval_if_form;

    else if (sv_eq(SYMBOL(vm_peek(vm)), sv_mk("let")))
        handler = eval_let_form;

    else if (sv_eq(SYMBOL(vm_peek(vm)), sv_mk("lambda")))
        handler = eval_lambda_form;

    else if (sv_eq(SYMBOL(vm_peek(vm)), sv_mk("quote")))
        handler = eval_quote_form;

    else if (sv_eq(SYMBOL(vm_peek(vm)), sv_mk("try")))
        handler = eval_try_form;

    if (handler) {
        vm_pop(vm);
        size_t argc = unpack_list(vm);
        handler(vm, argc);
        return true;
    }

    return false;
}

// Symbol (name), Node (value) -> Node
void eval_let_form(VM *vm, size_t argc) {
    if (argc != 2)
        vm_recover(vm);

    vm_swap(vm);

    if (vm_peek(vm)->kind != LISP_SYMBOL)
        vm_recover(vm);

    StringView name = SYMBOL(vm_peek(vm));
    vm_pop(vm);
    vm_eval_expr(vm);

    scope_define(VM_CURR_SCOPE(vm), name, vm_peek(vm));
}

// Node (condition), Node (is_true), Node (is_false) -> result
void eval_if_form(VM *vm, size_t argc) {
    if (argc != 3 && argc != 2)
        vm_recover(vm);

    if (argc == 2)
        vm_build_nil(vm);

    vm_rot(vm);
    vm_eval_expr(vm);

    bool is_positive = vm_peek(vm)->kind != LISP_NIL;
    vm_pop(vm);

    if (is_positive)
        vm_pop(vm);
    else
        vm_pop_prev(vm);

    vm_eval_expr(vm);
}

// Cons (Args list), Node (subexpr) -> Lambda
void eval_lambda_form(VM *vm, size_t argc) {
    if (argc != 2)
        vm_recover(vm);

    vm_swap(vm);

    StringViewDA args;
    bool is_variadic = false;
    da_init(args);
    
    if (vm_peek(vm)->kind != LISP_CONS && vm_peek(vm)->kind != LISP_SYMBOL) {
        da_free(args);
        vm_recover(vm);
    }

    // Variadic function with no positional arguments
    if (vm_peek(vm)->kind == LISP_SYMBOL) {
        is_variadic = true;
        da_push(args, SYMBOL(vm_peek(vm)));
    }

    // Function with at least one positional argument
    else {
        LispNode *curr = vm_peek(vm);
        for (; curr->kind == LISP_CONS; curr = CDR(curr)) {
            if (CAR(curr)->kind != LISP_SYMBOL) {
                da_free(args);
                vm_recover(vm);
            }
            da_push(args, SYMBOL(CAR(curr)));
        }

        if (curr->kind != LISP_SYMBOL && curr->kind != LISP_NIL) {
            da_free(args);
            vm_recover(vm);
        }

        if (curr->kind == LISP_SYMBOL) {
            is_variadic = true;
            da_push(args, SYMBOL(curr));
        }
    }
    vm_pop(vm);
    LispNode *subexpr = vm_peek(vm);

    vm_build_lambda(vm, args, is_variadic, subexpr, VM_CURR_SCOPE(vm));
    vm_pop_prev(vm);
}

// Node -> Node
void eval_try_form(VM *vm, size_t argc) {
    if (argc != 1)
        vm_recover(vm);

    jmp_buf env;
    vm_push_recovery(vm, &env);

    if (setjmp(env)) {
        vm_pop(vm);
        vm_build_integer(vm, 1);
    } else {
        vm_push(vm, da_at_end(vm->value_stack, 0));
        vm_eval_expr(vm);
        vm_pop(vm);
        vm_pop(vm);
        vm_build_nil(vm);
    }

    vm_pop_recovery(vm);
}

// Node -> Node
void eval_quote_form(VM *vm, size_t argc) {
    if (argc != 1)
        vm_recover(vm);
}

