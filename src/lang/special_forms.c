#include "special_forms.h"
#include "dynamic_array.h"
#include "object.h"
#include "scope.h"
#include "string_interner.h"
#include "vm.h"

#include <assert.h>

void special_forms_attach(VM *vm) {
    for (size_t i = 0; i < SPECIAL_FORMS_COUNT; i++)
        vm_register_special_form(vm, SPECIAL_FORMS[i].keyword, SPECIAL_FORMS[i].func);
}

// List -> Value
static void spk_let_form(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));
    Object *args = vm_peek(vm);
    size_t n = OBJ_LIST_SIZE(args);

    VM_RECOVER_IF_MSG(vm, n % 2 != 0, vm->singletons._VALUE_EXCEPTION,
                      "let needs a value for every name");

    vm_build_nil(vm);

    for (size_t i = 0; i + 1 < n; i += 2) {
        Object *name = OBJ_LIST_AT(args, i);
        VM_RECOVER_IF_MSG(vm, !OBJ_OFTYPE(name, TY_SYMBOL), vm->singletons._VALUE_EXCEPTION,
                          "let can only bind a symbol");

        vm_pop(vm);
        vm_push(vm, OBJ_LIST_AT(args, i + 1));
        vm_eval_object(vm);

        vm_scope_define(vm, OBJ_SYMBOL(name));
    }

    vm_pop_prev(vm);
}

// List -> Value
static void spk_set_form(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));
    Object *args = vm_peek(vm);
    size_t n = OBJ_LIST_SIZE(args);

    VM_RECOVER_IF_MSG(vm, n % 2 != 0, vm->singletons._VALUE_EXCEPTION,
                      "set needs a value for every name");

    size_t pairs = n / 2;

    for (size_t i = 0; i + 1 < n; i += 2) {
        Object *name = OBJ_LIST_AT(args, i);
        VM_RECOVER_IF_MSG(vm, !OBJ_OFTYPE(name, TY_SYMBOL), vm->singletons._VALUE_EXCEPTION,
                          "set can only assign to a symbol");

        vm_push(vm, OBJ_LIST_AT(args, i + 1));
        vm_eval_object(vm);
    }

    if (pairs == 0) {
        vm_pop(vm);
        vm_build_nil(vm);
        return;
    }

    for (size_t k = 0; k < pairs; k++) {
        Object *name = OBJ_LIST_AT(args, k * 2);
        Object *value = vm_peek_at(vm, pairs - 1 - k);
        bool result = scope_set(VM_CURR_SCOPE(vm), OBJ_SYMBOL(name), value);
        VM_RECOVER_IF(vm, !result, vm->singletons._UNDEFINED_EXCEPTION);
    }

    vm_pop_prev_n(vm, pairs);
}

// List -> Value
static void spk_if_form(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));
    Object *args = vm_peek(vm);
    size_t n = OBJ_LIST_SIZE(args);

    size_t i = 0;
    for (; i + 1 < n; i += 2) {
        vm_push(vm, OBJ_LIST_AT(args, i));
        vm_eval_object(vm);
        bool result = vm_cast_to_bool(vm);
        vm_pop(vm);

        if (result) {
            vm_push(vm, OBJ_LIST_AT(args, i + 1));
            vm_eval_object(vm);
            vm_pop_prev(vm);
            return;
        }
    }

    if (i < n) {
        vm_push(vm, OBJ_LIST_AT(args, i));
        vm_eval_object(vm);
        vm_pop_prev(vm);
        return;
    }

    vm_pop(vm);
    vm_build_nil(vm);
}

// List -> Value
static void spk_while_form(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));
    Object *args = vm_peek(vm);
    size_t n = OBJ_LIST_SIZE(args);
    VM_RECOVER_IF_MSG(vm, n < 1, vm->singletons._VALUE_EXCEPTION, "while needs a condition");

    Object *condition = OBJ_LIST_AT(args, 0);

    vm_build_nil(vm);

    vm_push(vm, condition);
    vm_eval_object(vm);
    bool truthy = vm_cast_to_bool(vm);

    while (truthy) {
        vm_pop(vm);

        // Fresh scope per iteration, the same rule for follows: a bare let binds
        // anew each turn instead of colliding, and a lambda made in the body
        // captures this turn alone. An empty body leaves the prior result.
        vm_build_scope(vm);
        for (size_t i = 1; i < n; i++) {
            vm_pop(vm);
            vm_push(vm, OBJ_LIST_AT(args, i));
            vm_eval_object(vm);
        }
        vm_pop_scope(vm);

        vm_push(vm, condition);
        vm_eval_object(vm);
        truthy = vm_cast_to_bool(vm);
    }

    vm_pop(vm);
    vm_pop_prev(vm);
}

// List -> Value
static void spk_for_form(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));
    Object *args = vm_peek(vm);
    size_t n = OBJ_LIST_SIZE(args);
    VM_RECOVER_IF(vm, n < 2, vm->singletons._VALUE_EXCEPTION);

    // The binding target is a bare Symbol for the single-name form or a
    // two-Symbol list (key value) for the keyed one. The shape alone tells them
    // apart, so the source list that follows may still be a bare symbol.
    Object *target = OBJ_LIST_AT(args, 0);
    VM_RECOVER_IF(vm, !OBJ_OFTYPE(target, TY_SYMBOL | TY_LIST), vm->singletons._VALUE_EXCEPTION);

    Object *key_sym = NULL;
    Object *val_sym;
    if (OBJ_OFTYPE(target, TY_SYMBOL)) {
        val_sym = target;
    } else {
        VM_RECOVER_IF(vm, OBJ_LIST_SIZE(target) != 2, vm->singletons._VALUE_EXCEPTION);
        key_sym = OBJ_LIST_AT(target, 0);
        val_sym = OBJ_LIST_AT(target, 1);
        VM_RECOVER_IF(vm, !OBJ_OFTYPE(key_sym, TY_SYMBOL) || !OBJ_OFTYPE(val_sym, TY_SYMBOL),
                      vm->singletons._VALUE_EXCEPTION);
        VM_RECOVER_IF(vm, OBJ_SYMBOL(key_sym) == OBJ_SYMBOL(val_sym),
                      vm->singletons._VALUE_EXCEPTION);
    }

    vm_push(vm, OBJ_LIST_AT(args, 1));
    vm_eval_object(vm);
    VM_RECOVER_IF(vm, !OBJ_OFTYPE(vm_peek(vm), TY_LIST), vm->singletons._TYPE_EXCEPTION);

    Object *items = vm_peek(vm);

    vm_build_nil(vm);

    // Size re-read each step, so a body that shrinks the list can't index past the end.
    for (size_t i = 0; i < OBJ_LIST_SIZE(items); i++) {
        // Fresh scope per iteration, so a lambda in the body captures this step's binding.
        vm_build_scope(vm);

        if (key_sym) {
            vm_build_integer(vm, (Integer)i);
            vm_scope_define(vm, OBJ_SYMBOL(key_sym));
            vm_pop(vm);
        }

        vm_push(vm, OBJ_LIST_AT(items, i));
        vm_scope_define(vm, OBJ_SYMBOL(val_sym));
        vm_pop(vm);

        for (size_t b = 2; b < n; b++) {
            vm_pop(vm);
            vm_push(vm, OBJ_LIST_AT(args, b));
            vm_eval_object(vm);
        }

        vm_pop_scope(vm);
    }

    vm_pop_prev_n(vm, 2);
}

static bool lambda_has_arg(Object *lambda, StringName name) {
    for (size_t i = 0; i < OBJ_LAMBDA_ARGS(lambda).size; i++)
        if (da_at(OBJ_LAMBDA_ARGS(lambda), i) == name)
            return true;
    return false;
}

// List -> Value
static void spk_lambda_form(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));
    Object *args = vm_peek(vm);
    size_t n = OBJ_LIST_SIZE(args);
    VM_RECOVER_IF(vm, n < 2, vm->singletons._VALUE_EXCEPTION);

    Object *params = OBJ_LIST_AT(args, 0);
    VM_RECOVER_IF(vm, !OBJ_OFTYPE(params, TY_SYMBOL | TY_LIST), vm->singletons._VALUE_EXCEPTION);

    // One body expression is stored as-is; several are wrapped in a synthesized
    // (begin ...) so the call reuses begin's sequencing and its scope, adding no
    // path to vm_call_lambda. The begin symbol is interned by name the way the
    // parser interns quote, so it need not be a reserved VM symbol.
    Object *body;
    bool wrapped = n > 2;
    if (!wrapped) {
        body = OBJ_LIST_AT(args, 1);
    } else {
        vm_build_symbol(vm, si_get(vm->si, "begin"));
        for (size_t i = 1; i < n; i++)
            vm_push(vm, OBJ_LIST_AT(args, i));
        vm_pack_list(vm, n);
        body = vm_peek(vm);
    }

    vm_build_lambda(vm, false, body, VM_CURR_SCOPE(vm));
    Object *lambda = vm_peek(vm);

    bool is_variadic = false;

    if (OBJ_OFTYPE(params, TY_SYMBOL)) {
        is_variadic = true;
        da_push(OBJ_LAMBDA_ARGS(lambda), OBJ_SYMBOL(params));
    } else {
        StringName var_marker = VM_SYM(vm, Var);
        size_t np = OBJ_LIST_SIZE(params);

        for (size_t i = 0; i < np; i++) {
            Object *param = OBJ_LIST_AT(params, i);
            VM_RECOVER_IF(vm, !OBJ_OFTYPE(param, TY_SYMBOL), vm->singletons._VALUE_EXCEPTION);

            if (OBJ_SYMBOL(param) == var_marker) {
                VM_RECOVER_IF(vm, i != np - 2, vm->singletons._VALUE_EXCEPTION);

                Object *rest = OBJ_LIST_AT(params, np - 1);
                VM_RECOVER_IF(vm, !OBJ_OFTYPE(rest, TY_SYMBOL), vm->singletons._VALUE_EXCEPTION);
                VM_RECOVER_IF(vm, lambda_has_arg(lambda, OBJ_SYMBOL(rest)),
                              vm->singletons._VALUE_EXCEPTION);

                is_variadic = true;
                da_push(OBJ_LAMBDA_ARGS(lambda), OBJ_SYMBOL(rest));
                break;
            }

            VM_RECOVER_IF(vm, lambda_has_arg(lambda, OBJ_SYMBOL(param)),
                          vm->singletons._VALUE_EXCEPTION);
            da_push(OBJ_LAMBDA_ARGS(lambda), OBJ_SYMBOL(param));
        }
    }

    OBJ_LAMBDA_IS_VARIADIC(lambda) = is_variadic;
    if (wrapped)
        vm_pop_prev(vm); // discard the synthesized (begin ...) list
    vm_pop_prev(vm);     // discard the args list
}

// List -> Value
// The caught kinds are evaluated before the frame is pushed, so an error while
// producing one reaches the enclosing handler. The frame is then pushed over the
// values the handler needs, so unwinding discards the rest on its own.
static void spk_try_form(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));
    Object *args = vm_peek(vm);
    size_t n = OBJ_LIST_SIZE(args);

    VM_RECOVER_IF(vm, n == 0, vm->singletons._VALUE_EXCEPTION);

    // One kind is written bare, several as a list; either way each is evaluated
    // to a Symbol and packed into a List the handler scans on unwind.
    Object *target = OBJ_LIST_AT(args, 0);
    size_t nkinds;
    if (OBJ_OFTYPE(target, TY_LIST)) {
        nkinds = OBJ_LIST_SIZE(target);
        VM_RECOVER_IF(vm, nkinds == 0, vm->singletons._VALUE_EXCEPTION);
        for (size_t i = 0; i < nkinds; i++) {
            vm_push(vm, OBJ_LIST_AT(target, i));
            vm_eval_object(vm);
            VM_RECOVER_IF(vm, !OBJ_OFTYPE(vm_peek(vm), TY_SYMBOL), vm->singletons._VALUE_EXCEPTION);
        }
    } else {
        vm_push(vm, target);
        vm_eval_object(vm);
        VM_RECOVER_IF(vm, !OBJ_OFTYPE(vm_peek(vm), TY_SYMBOL), vm->singletons._VALUE_EXCEPTION);
        nkinds = 1;
    }
    vm_pack_list(vm, nkinds);

    jmp_buf env;
    vm_push_recovery(vm, &env);

    if (setjmp(env)) {
        // Unwound back to List (args), List (kinds). Pop this frame first so an
        // uncaught kind reaches the enclosing handler (vm_recover raises into the
        // topmost frame). Kinds match through vm_eq, the equality the language
        // gives =, borrowed rather than a symbol compare rebuilt here.
        vm_pop_recovery(vm);

        Object *kinds = vm_peek(vm);
        Object *raised = vm_exception_kind(vm);
        bool matched = false;
        for (size_t i = 0; i < OBJ_LIST_SIZE(kinds); i++) {
            vm_push(vm, OBJ_LIST_AT(kinds, i));
            vm_push(vm, raised);
            matched = vm_eq(vm);
            vm_pop(vm);
            if (matched)
                break;
        }

        if (!matched)
            vm_recover(vm, vm->exception);

        // A caught try yields what was raised: the bare kind Symbol, or the
        // Exception carrying its value. Replace the kinds list with it.
        vm_pop(vm);
        vm_push(vm, vm->exception);
        vm_pop_prev(vm);
        return;
    }

    vm_build_scope(vm);
    vm_build_nil(vm);

    for (size_t i = 1; i < n; i++) {
        vm_pop(vm);
        vm_push(vm, OBJ_LIST_AT(args, i));
        vm_eval_object(vm);
    }

    vm_pop_scope(vm);
    vm_pop_recovery(vm);
    vm_pop_prev_n(vm, 2);
}

// List -> Value
static void spk_quote_form(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));
    Object *args = vm_peek(vm);
    VM_RECOVER_IF_MSG(vm, OBJ_LIST_SIZE(args) != 1, vm->singletons._VALUE_EXCEPTION,
                      "quote takes one expression");

    Object *expr = OBJ_LIST_AT(args, 0);
    vm_pop(vm);
    vm_push(vm, expr);
}

// List -> Value
static void spk_begin_form(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));
    Object *args = vm_peek(vm);

    vm_build_scope(vm);
    vm_build_nil(vm);

    OBJ_LIST_FOREACH(curr, args)
        vm_pop(vm);
        vm_push(vm, curr);
        vm_eval_object(vm);
    OBJ_END_LIST_FOREACH

    vm_pop_prev(vm);
    vm_pop_scope(vm);
}

// List -> Value
static void spk_and_form(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));
    Object *args = vm_peek(vm);

    bool result = true;
    OBJ_LIST_FOREACH(curr, args)
        vm_push(vm, curr);
        vm_eval_object(vm);
        vm_cast_to_bool(vm);
        result = OBJ_BOOL(vm_peek(vm));
        vm_pop(vm);
        if (!result)
            break;
    OBJ_END_LIST_FOREACH

    vm_pop(vm);
    vm_build_bool(vm, result);
}

// List -> Value
static void spk_or_form(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));
    Object *args = vm_peek(vm);

    bool result = false;
    OBJ_LIST_FOREACH(curr, args)
        vm_push(vm, curr);
        vm_eval_object(vm);
        vm_cast_to_bool(vm);
        result = OBJ_BOOL(vm_peek(vm));
        vm_pop(vm);
        if (result)
            break;
    OBJ_END_LIST_FOREACH

    vm_pop(vm);
    vm_build_bool(vm, result);
}

const SpecialFormDef SPECIAL_FORMS[] = {
    {"let", spk_let_form},     {"set", spk_set_form},       {"if", spk_if_form},
    {"while", spk_while_form}, {"lambda", spk_lambda_form}, {"begin", spk_begin_form},
    {"quote", spk_quote_form}, {"try", spk_try_form},       {"and", spk_and_form},
    {"or", spk_or_form},       {"for", spk_for_form}};

const size_t SPECIAL_FORMS_COUNT = sizeof(SPECIAL_FORMS) / sizeof(SPECIAL_FORMS[0]);
