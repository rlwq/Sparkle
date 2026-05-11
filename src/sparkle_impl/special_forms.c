#include "special_forms.h"
#include "lisp_node.h"
#include "string_interner.h"
#include "vm.h"
#include <stdio.h>

// Node (Args), Node (Head) -> Node (Maybe :3)
bool try_dispatch_special_form(VM *vm) {
    ASSERT_HAS(vm, 2);

    if (vm_peek(vm)->kind != LISP_SYMBOL)
        return false;

    SpecialFormHandler handler = NULL;

    for (size_t i = 0; i < SPECIAL_FORMS_COUNT; i++) {
        if (SYMBOL(vm_peek(vm)) == SPECIAL_FORMS[i].keyword) {
            handler = SPECIAL_FORMS[i].func;
            break;
        }
    }

    if (handler) {
        vm_pop(vm);
        size_t argc = vm_unpack_list(vm);
        handler(vm, argc);
        return true;
    }

    return false;
}

// Symbol (name), Node (value) -> Node
void eval_let_form(VM *vm, size_t argc) {
    VM_RECOVER_IF(vm, argc != 2, INVALID_SPECIAL_FORM);

    vm_swap(vm);

    VM_RECOVER_IF(vm, !NODE_IS(vm_peek(vm), LISP_SYMBOL), INVALID_SPECIAL_FORM);

    StringName name = SYMBOL(vm_peek(vm));
    vm_pop(vm);
    vm_eval_node(vm);
    VM_RECOVER_IF(vm, !vm_scope_define(vm, name), SYMBOL_REBINDING);
}

// Symbol (name), Node (value) -> Node
void eval_set_form(VM *vm, size_t argc) {
    VM_RECOVER_IF(vm, argc != 2, INVALID_SPECIAL_FORM);

    vm_swap(vm);

    VM_RECOVER_IF(vm, !NODE_IS(vm_peek(vm), LISP_SYMBOL), INVALID_SPECIAL_FORM);

    StringName name = SYMBOL(vm_peek(vm));
    vm_pop(vm);
    vm_eval_node(vm);
    vm_scope_set(vm, name);
}

// Node (condition), Node (is_true), Node (is_false) -> result
void eval_if_form(VM *vm, size_t argc) {
    VM_RECOVER_IF(vm, argc != 2 && argc != 3, INVALID_SPECIAL_FORM);

    if (argc == 2)
        vm_build_nil(vm);

    vm_rot(vm);
    vm_eval_node(vm);
    vm_cast_to_bool(vm);

    bool is_positive = BOOL(vm_peek(vm));
    vm_pop(vm);

    if (is_positive)
        vm_pop(vm);
    else
        vm_pop_prev(vm);

    vm_eval_node(vm);
}

// Node (condition), Node (body) -> Nil
void eval_while_form(VM *vm, size_t argc) {
    VM_RECOVER_IF(vm, argc != 2, INVALID_SPECIAL_FORM);

    vm_dup_prev(vm);
    vm_eval_node(vm);
    bool result = vm_cast_to_bool(vm);

    while (result) {
        vm_pop(vm);
        vm_dup(vm);
        vm_eval_node(vm);
        vm_pop(vm);

        vm_dup_prev(vm);
        vm_eval_node(vm);
        result = vm_cast_to_bool(vm);
    }

    vm_pop_n(vm, 3);

    vm_build_nil(vm);
}

// Cons (Args list), Node (subexpr) -> Lambda
void eval_lambda_form(VM *vm, size_t argc) {
    VM_RECOVER_IF(vm, argc != 2, INVALID_SPECIAL_FORM);

    vm_swap(vm);

    LambdaArgs args;
    bool is_variadic = false;
    da_init(args);

    if (!IS_LISTFUL(vm_peek(vm)) && !NODE_IS(vm_peek(vm), LISP_SYMBOL)) {
        da_free(args);
        vm_recover(vm, INVALID_SPECIAL_FORM);
    }

    // Variadic function with no positional arguments
    if (NODE_IS(vm_peek(vm), LISP_SYMBOL)) {
        is_variadic = true;
        da_push(args, SYMBOL(vm_peek(vm)));
    }

    // Function with at least one positional argument
    else {
        LispNode *curr = vm_peek(vm);
        for (; curr->kind == LISP_CONS; curr = CDR(curr)) {
            if (!NODE_IS(CAR(curr), LISP_SYMBOL)) {
                da_free(args);
                vm_recover(vm, INVALID_SPECIAL_FORM);
            }
            da_push(args, SYMBOL(CAR(curr)));
        }

        if (!NODE_IS(curr, LISP_NIL) && !NODE_IS(curr, LISP_SYMBOL)) {
            da_free(args);
            vm_recover(vm, INVALID_SPECIAL_FORM);
        }

        if (NODE_IS(curr, LISP_SYMBOL)) {
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
    VM_RECOVER_IF(vm, argc != 1, INVALID_SPECIAL_FORM);

    jmp_buf env;
    vm_push_recovery(vm, &env);

    if (setjmp(env)) {
        vm_pop(vm);
        vm_build_exception(vm, vm->exception);
    } else {
        vm_push(vm, da_at_end(vm->value_stack, 0));
        vm_eval_node(vm);
        vm_pop_n(vm, 2);
        vm_build_nil(vm);
    }

    vm_pop_recovery(vm);
}

// Node -> Node
void eval_quote_form(VM *vm, size_t argc) {
    VM_RECOVER_IF(vm, argc != 1, INVALID_SPECIAL_FORM);
}

void eval_begin_form(VM *vm, size_t argc) {
    vm_build_scope(vm);

    for (size_t i = 0; i + 1 < argc; i++) {
        vm_push(vm, da_at_end(vm->value_stack, argc - 1 - i));
        vm_eval_node(vm);
        vm_pop(vm);
    }

    if (argc > 0)
        vm_eval_node(vm);
    else
        vm_build_nil(vm);

    if (argc > 0)
        vm_pop_prev_n(vm, argc - 1);

    vm_pop_scope(vm);
}

SpecialFormDef SPECIAL_FORMS[] = {{"let", eval_let_form},       {"set", eval_set_form},
                                  {"lambda", eval_lambda_form}, {"quote", eval_quote_form},
                                  {"try", eval_try_form},       {"if", eval_if_form},
                                  {"begin", eval_begin_form},   {"while", eval_while_form}};

size_t SPECIAL_FORMS_COUNT = sizeof(SPECIAL_FORMS) / sizeof(SPECIAL_FORMS[0]);
