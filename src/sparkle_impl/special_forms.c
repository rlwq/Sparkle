#include "special_forms.h"
#include "dynamic_array.h"
#include "object.h"
#include "string_interner.h"
#include "vm.h"
#include <stdio.h>

// Node (Args), Node (Head) -> Node (Maybe :3)
bool try_dispatch_special_form(VM *vm) {

    if (!OFTYPE(vm_peek(vm), TY_SYMBOL))
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
    VM_RECOVER_IF(vm, argc != 2, vm->singletons._VALUE_EXCEPTION);

    vm_swap(vm);

    VM_RECOVER_IF(vm, !OFTYPE(vm_peek(vm), TY_SYMBOL), vm->singletons._VALUE_EXCEPTION);

    StringName name = SYMBOL(vm_peek(vm));
    vm_pop(vm);
    vm_eval_node(vm);
    VM_RECOVER_IF(vm, !vm_scope_define(vm, name), vm->singletons._REBINDING_EXCEPTION);
}

// Symbol (name), Node (value) -> Node
void eval_set_form(VM *vm, size_t argc) {
    VM_RECOVER_IF(vm, argc != 2, vm->singletons._VALUE_EXCEPTION);

    vm_swap(vm);

    VM_RECOVER_IF(vm, !OFTYPE(vm_peek(vm), TY_SYMBOL), vm->singletons._VALUE_EXCEPTION);

    StringName name = SYMBOL(vm_peek(vm));
    vm_pop(vm);
    vm_eval_node(vm);
    vm_scope_set(vm, name);
}

// Node (condition), Node (is_true), Node (is_false) -> result
void eval_if_form(VM *vm, size_t argc) {
    VM_RECOVER_IF(vm, argc != 2 && argc != 3, vm->singletons._VALUE_EXCEPTION);

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
    VM_RECOVER_IF(vm, argc != 2, vm->singletons._VALUE_EXCEPTION);

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
    VM_RECOVER_IF(vm, argc != 2, vm->singletons._VALUE_EXCEPTION);

    vm_build_lambda(vm, (LambdaArgs)da_empty, false, vm_peek(vm), VM_CURR_SCOPE(vm));
    da_init(LAMBDA_ARGS(vm_peek(vm)));
    vm_pop_prev(vm);

    bool is_variadic = false;

    vm_expect2(vm, TY_SYMBOL | TY_LISTFUL, TY_ANY);

    // Variadic function with no positional arguments
    if (OFTYPE(vm_peek(vm), TY_SYMBOL)) {
        is_variadic = true;
        da_push(LAMBDA_ARGS(vm_peek(vm)), SYMBOL(vm_prev(vm)));
        goto end;
    }

    // Function with at least one positional argument
    Object *curr = vm_prev(vm);
    for (; curr->kind == KIND_CONS; curr = CDR(curr)) {
        VM_RECOVER_IF(vm, !OFTYPE(CAR(curr), TY_SYMBOL), vm->singletons._VALUE_EXCEPTION);
        da_push(LAMBDA_ARGS(vm_peek(vm)), SYMBOL(CAR(curr)));
    }

    VM_RECOVER_IF(vm, !OFTYPE(curr, TY_NIL | TY_SYMBOL), vm->singletons._VALUE_EXCEPTION);

    if (OFTYPE(curr, TY_SYMBOL)) {
        is_variadic = true;
        da_push(LAMBDA_ARGS(vm_peek(vm)), SYMBOL(curr));
    }

end:
    LAMBDA_IS_VARIADIC(vm_peek(vm)) = is_variadic;
    vm_pop_prev(vm);
}

// Node -> Node
void eval_try_form(VM *vm, size_t argc) {
    VM_RECOVER_IF(vm, argc != 1, vm->singletons._VALUE_EXCEPTION);

    jmp_buf env;
    vm_push_recovery(vm, &env);

    if (setjmp(env)) {
        vm_pop(vm);
        vm_push(vm, vm->exception);
    } else {
        vm_dup(vm);
        vm_eval_node(vm);
        vm_pop_n(vm, 2);
        vm_build_nil(vm);
    }

    vm_pop_recovery(vm);
}

// Node -> Node
void eval_quote_form(VM *vm, size_t argc) {
    VM_RECOVER_IF(vm, argc != 1, vm->singletons._VALUE_EXCEPTION);
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
