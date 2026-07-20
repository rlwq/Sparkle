#include "special_forms.h"
#include "dynamic_array.h"
#include "object.h"
#include "scope.h"
#include "string_interner.h"
#include "vm.h"

void special_forms_attach(VM *vm) {
    for (size_t i = 0; i < SPECIAL_FORMS_COUNT; i++)
        SPECIAL_FORMS[i].keyword = si_get(vm->si, SPECIAL_FORMS[i].keyword);

    vm->try_special = try_dispatch_special_form;
}

bool try_dispatch_special_form(VM *vm, StringName name) {
    for (size_t i = 0; i < SPECIAL_FORMS_COUNT; i++) {
        if (name == SPECIAL_FORMS[i].keyword) {
            SPECIAL_FORMS[i].func(vm);
            return true;
        }
    }

    return false;
}

void rkl_let_form(VM *vm) {
    Object *args = vm_peek(vm);
    size_t n = LIST_SIZE(args);

    VM_RECOVER_IF(vm, n % 2 != 0, vm->singletons._VALUE_EXCEPTION);

    for (size_t i = 0; i + 1 < n; i += 2) {
        Object *name = LIST_AT(args, i);
        VM_RECOVER_IF(vm, !OFTYPE(name, TY_SYMBOL), vm->singletons._VALUE_EXCEPTION);

        vm_push(vm, LIST_AT(args, i + 1));
        vm_eval_node(vm);

        vm_scope_define(vm, SYMBOL(name));
        vm_pop(vm);
    }

    vm_pop(vm);
    vm_build_nil(vm);
}

// All exprs are evaluated first (left to right), then all assignments are
// performed against the pre-evaluation bindings - this is what lets
// `(set a b b a)` swap instead of clobbering b before it's read.
void rkl_set_form(VM *vm) {
    Object *args = vm_peek(vm);
    size_t n = LIST_SIZE(args);

    VM_RECOVER_IF(vm, n % 2 != 0, vm->singletons._VALUE_EXCEPTION);

    size_t pairs = n / 2;

    for (size_t i = 0; i + 1 < n; i += 2) {
        Object *name = LIST_AT(args, i);
        VM_RECOVER_IF(vm, !OFTYPE(name, TY_SYMBOL), vm->singletons._VALUE_EXCEPTION);

        vm_push(vm, LIST_AT(args, i + 1));
        vm_eval_node(vm);
    }

    if (pairs == 0) {
        vm_pop(vm);
        vm_build_nil(vm);
        return;
    }

    for (size_t k = 0; k < pairs; k++) {
        Object *name = LIST_AT(args, k * 2);
        Object *value = vm_peek_at(vm, pairs - 1 - k);
        bool result = scope_set(VM_CURR_SCOPE(vm), SYMBOL(name), value);
        VM_RECOVER_IF(vm, !result, vm->singletons._UNDEFINED_EXCEPTION);
    }

    vm_pop_prev_n(vm, pairs);
}

void rkl_if_form(VM *vm) {
    Object *args = vm_peek(vm);
    size_t n = LIST_SIZE(args);

    size_t i = 0;
    for (; i + 1 < n; i += 2) {
        vm_push(vm, LIST_AT(args, i));
        vm_eval_node(vm);
        bool result = vm_cast_to_bool(vm);
        vm_pop(vm);

        if (result) {
            vm_push(vm, LIST_AT(args, i + 1));
            vm_eval_node(vm);
            vm_pop_prev(vm);
            return;
        }
    }

    if (i < n) {
        vm_push(vm, LIST_AT(args, i));
        vm_eval_node(vm);
        vm_pop_prev(vm);
        return;
    }

    vm_pop(vm);
    vm_build_nil(vm);
}

void rkl_while_form(VM *vm) {
    Object *args = vm_peek(vm);
    VM_RECOVER_IF(vm, LIST_SIZE(args) != 2, vm->singletons._VALUE_EXCEPTION);

    Object *condition = LIST_AT(args, 0);
    Object *body = LIST_AT(args, 1);

    vm_push(vm, condition);
    vm_eval_node(vm);
    bool result = vm_cast_to_bool(vm);

    while (result) {
        vm_pop(vm);

        vm_push(vm, body);
        vm_eval_node(vm);
        vm_pop(vm);

        vm_push(vm, condition);
        vm_eval_node(vm);
        result = vm_cast_to_bool(vm);
    }

    vm_pop(vm);
    vm_pop(vm);
    vm_build_nil(vm);
}

static bool lambda_has_arg(Object *lambda, StringName name) {
    for (size_t i = 0; i < LAMBDA_ARGS(lambda).size; i++)
        if (da_at(LAMBDA_ARGS(lambda), i) == name)
            return true;
    return false;
}

void rkl_lambda_form(VM *vm) {
    Object *args = vm_peek(vm);
    VM_RECOVER_IF(vm, LIST_SIZE(args) != 2, vm->singletons._VALUE_EXCEPTION);

    Object *params = LIST_AT(args, 0);
    Object *body = LIST_AT(args, 1);

    VM_RECOVER_IF(vm, !OFTYPE(params, TY_SYMBOL | TY_LIST), vm->singletons._VALUE_EXCEPTION);

    vm_build_lambda(vm, false, body, VM_CURR_SCOPE(vm));
    Object *lambda = vm_peek(vm);

    bool is_variadic = false;

    if (OFTYPE(params, TY_SYMBOL)) {
        is_variadic = true;
        da_push(LAMBDA_ARGS(lambda), SYMBOL(params));
    } else {
        StringName var_marker = vm->si->prebuilt._Var;
        size_t np = LIST_SIZE(params);

        for (size_t i = 0; i < np; i++) {
            Object *param = LIST_AT(params, i);
            VM_RECOVER_IF(vm, !OFTYPE(param, TY_SYMBOL), vm->singletons._VALUE_EXCEPTION);

            if (SYMBOL(param) == var_marker) {
                VM_RECOVER_IF(vm, i != np - 2, vm->singletons._VALUE_EXCEPTION);

                Object *rest = LIST_AT(params, np - 1);
                VM_RECOVER_IF(vm, !OFTYPE(rest, TY_SYMBOL), vm->singletons._VALUE_EXCEPTION);
                VM_RECOVER_IF(vm, lambda_has_arg(lambda, SYMBOL(rest)),
                              vm->singletons._VALUE_EXCEPTION);

                is_variadic = true;
                da_push(LAMBDA_ARGS(lambda), SYMBOL(rest));
                break;
            }

            VM_RECOVER_IF(vm, lambda_has_arg(lambda, SYMBOL(param)),
                          vm->singletons._VALUE_EXCEPTION);
            da_push(LAMBDA_ARGS(lambda), SYMBOL(param));
        }
    }

    LAMBDA_IS_VARIADIC(lambda) = is_variadic;
    vm_pop_prev(vm);
}

void rkl_try_form(VM *vm) {
    Object *args = vm_peek(vm);
    VM_RECOVER_IF(vm, LIST_SIZE(args) != 1, vm->singletons._VALUE_EXCEPTION);

    Object *expr = LIST_AT(args, 0);
    vm_pop(vm);
    vm_push(vm, expr);

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
    Object *args = vm_peek(vm);
    VM_RECOVER_IF(vm, LIST_SIZE(args) != 1, vm->singletons._VALUE_EXCEPTION);

    Object *expr = LIST_AT(args, 0);
    vm_pop(vm);
    vm_push(vm, expr);
}

void rkl_begin_form(VM *vm) {
    Object *args = vm_peek(vm);

    vm_build_scope(vm);
    vm_build_nil(vm);

    LIST_FOREACH(curr, args)
    vm_pop(vm);
    vm_push(vm, curr);
    vm_eval_node(vm);
    END_LIST_FOREACH

    vm_pop_prev(vm);
    vm_pop_scope(vm);
}

void rkl_and_form(VM *vm) {
    Object *args = vm_peek(vm);

    bool result = true;
    LIST_FOREACH(curr, args)
    vm_push(vm, curr);
    vm_eval_node(vm);
    vm_cast_to_bool(vm);
    result = BOOL(vm_peek(vm));
    vm_pop(vm);
    if (!result)
        break;
    END_LIST_FOREACH

    vm_pop(vm);
    vm_build_bool(vm, result);
}

void rkl_or_form(VM *vm) {
    Object *args = vm_peek(vm);

    bool result = false;
    LIST_FOREACH(curr, args)
    vm_push(vm, curr);
    vm_eval_node(vm);
    vm_cast_to_bool(vm);
    result = BOOL(vm_peek(vm));
    vm_pop(vm);
    if (result)
        break;
    END_LIST_FOREACH

    vm_pop(vm);
    vm_build_bool(vm, result);
}

SpecialFormDef SPECIAL_FORMS[] = {{"let", rkl_let_form},       {"set", rkl_set_form},
                                  {"if", rkl_if_form},         {"while", rkl_while_form},
                                  {"lambda", rkl_lambda_form}, {"begin", rkl_begin_form},
                                  {"quote", rkl_quote_form},   {"try", rkl_try_form},
                                  {"and", rkl_and_form},       {"or", rkl_or_form}};

size_t SPECIAL_FORMS_COUNT = sizeof(SPECIAL_FORMS) / sizeof(SPECIAL_FORMS[0]);
