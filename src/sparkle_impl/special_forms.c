#include "special_forms.h"
#include "dynamic_array.h"
#include "object.h"
#include "string_interner.h"
#include "vm.h"
#include <stdio.h>

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
        handler(vm);
        return true;
    }

    return false;
}

void rkl_let_form(VM *vm) {
    Object *curr = vm_peek(vm);
    for (; OFTYPE(curr, TY_CONS) && OFTYPE(CDR(curr), TY_CONS); curr = CDR(CDR(curr))) {
        VM_RECOVER_IF(vm, !OFTYPE(CAR(curr), TY_SYMBOL), vm->singletons._VALUE_EXCEPTION);

        StringName name = SYMBOL(CAR(curr));
        Object *value = CAR(CDR(curr));

        vm_push(vm, value);
        vm_eval_node(vm);

        vm_scope_define(vm, name);
        vm_pop(vm);
    }

    VM_RECOVER_IF(vm, !OFTYPE(curr, TY_NIL), vm->singletons._VALUE_EXCEPTION);

    vm_pop(vm);
    vm_build_nil(vm);
}

void rkl_set_form(VM *vm) {
    Object *curr = vm_peek(vm);
    for (; OFTYPE(curr, TY_CONS) && OFTYPE(CDR(curr), TY_CONS); curr = CDR(CDR(curr))) {
        VM_RECOVER_IF(vm, !OFTYPE(CAR(curr), TY_SYMBOL), vm->singletons._VALUE_EXCEPTION);

        StringName name = SYMBOL(CAR(curr));
        Object *value = CAR(CDR(curr));

        vm_push(vm, value);
        vm_eval_node(vm);

        vm_scope_set(vm, name);
        vm_pop(vm);
    }

    VM_RECOVER_IF(vm, !OFTYPE(curr, TY_NIL), vm->singletons._VALUE_EXCEPTION);

    vm_pop(vm);
    vm_build_nil(vm);
}

void rkl_if_form(VM *vm) {
    Object *curr = vm_peek(vm);
    for (; OFTYPE(curr, TY_CONS) && OFTYPE(CDR(curr), TY_CONS); curr = CDR(CDR(curr))) {
        vm_push(vm, CAR(curr));
        vm_eval_node(vm);
        bool result = vm_cast_to_bool(vm);
        vm_pop(vm);

        if (result) {
            vm_push(vm, CAR(CDR(curr)));
            vm_eval_node(vm);
            vm_pop_prev(vm);
            return;
        }
    }

    if (OFTYPE(curr, TY_NIL)) {
        vm_pop(vm);
        vm_build_nil(vm);
        return;
    }

    vm_push(vm, CAR(curr));
    vm_eval_node(vm);
    vm_pop_prev(vm);
}

void rkl_while_form(VM *vm) {
    size_t argc = vm_unpack_list(vm);
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

void rkl_lambda_form(VM *vm) {
    size_t argc = vm_unpack_list(vm);
    VM_RECOVER_IF(vm, argc != 2, vm->singletons._VALUE_EXCEPTION);

    vm_build_lambda(vm, false, vm_peek(vm), VM_CURR_SCOPE(vm));
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

void rkl_try_form(VM *vm) {
    size_t argc = vm_unpack_list(vm);
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

void rkl_quote_form(VM *vm) {
    size_t argc = vm_unpack_list(vm);
    VM_RECOVER_IF(vm, argc != 1, vm->singletons._VALUE_EXCEPTION);
}

void rkl_begin_form(VM *vm) {
    vm_build_scope(vm);
    vm_build_nil(vm);
    LIST_ITER(vm, curr, vm_prev(vm))
        vm_pop(vm);
        vm_push(vm, CAR(curr));
        vm_eval_node(vm);
    END_LIST_ITER(vm, curr)
    vm_pop_prev(vm);
    vm_pop_scope(vm);
}

void rkl_and_form(VM *vm) {
    bool result = true;
    for (Object *curr = vm_peek(vm); OFTYPE(curr, TY_CONS); curr = CDR(curr)) {
        vm_push(vm, CAR(curr));
        vm_eval_node(vm);
        vm_cast_to_bool(vm);
        result = BOOL(vm_peek(vm));
        vm_pop(vm);
        if (!result)
            break;
    }
    vm_pop(vm);
    vm_build_bool(vm, result);
}

void rkl_or_form(VM *vm) {
    bool result = false;
    for (Object *curr = vm_peek(vm); OFTYPE(curr, TY_CONS); curr = CDR(curr)) {
        vm_push(vm, CAR(curr));
        vm_eval_node(vm);
        vm_cast_to_bool(vm);
        result = BOOL(vm_peek(vm));
        vm_pop(vm);
        if (result)
            break;
    }
    vm_pop(vm);
    vm_build_bool(vm, result);
}

SpecialFormDef SPECIAL_FORMS[] = {{"let", rkl_let_form},       {"set", rkl_set_form},
                                  {"if", rkl_if_form},         {"while", rkl_while_form},
                                  {"lambda", rkl_lambda_form}, {"begin", rkl_begin_form},
                                  {"quote", rkl_quote_form},   {"try", rkl_try_form},
                                  {"and", rkl_and_form},       {"or", rkl_or_form}};

size_t SPECIAL_FORMS_COUNT = sizeof(SPECIAL_FORMS) / sizeof(SPECIAL_FORMS[0]);
