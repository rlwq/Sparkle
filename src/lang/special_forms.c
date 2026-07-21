#include "special_forms.h"
#include "dynamic_array.h"
#include "object.h"
#include "scope.h"
#include "string_interner.h"
#include "vm.h"

void special_forms_attach(VM *vm) {
    for (size_t i = 0; i < SPECIAL_FORMS_COUNT; i++)
        vm_register_special_form(vm, SPECIAL_FORMS[i].keyword, SPECIAL_FORMS[i].func);
}

// A running result sits on the stack from the start: Nil stands in until the
// first binding, and each one drops the previous value and leaves its own, so
// whatever remains at the end is the last bound value.
static void spk_let_form(VM *vm) {
    Object *args = vm_peek(vm);
    size_t n = OBJ_LIST_SIZE(args);

    VM_RECOVER_IF(vm, n % 2 != 0, vm->singletons._VALUE_EXCEPTION);

    vm_build_nil(vm);

    for (size_t i = 0; i + 1 < n; i += 2) {
        Object *name = OBJ_LIST_AT(args, i);
        VM_RECOVER_IF(vm, !OBJ_OFTYPE(name, TY_SYMBOL), vm->singletons._VALUE_EXCEPTION);

        vm_pop(vm);
        vm_push(vm, OBJ_LIST_AT(args, i + 1));
        vm_eval_object(vm);

        vm_scope_define(vm, OBJ_SYMBOL(name));
    }

    vm_pop_prev(vm);
}

// All exprs are evaluated first (left to right), then all assignments are
// performed against the pre-evaluation bindings - this is what lets
// `(set a b b a)` swap instead of clobbering b before it's read.
static void spk_set_form(VM *vm) {
    Object *args = vm_peek(vm);
    size_t n = OBJ_LIST_SIZE(args);

    VM_RECOVER_IF(vm, n % 2 != 0, vm->singletons._VALUE_EXCEPTION);

    size_t pairs = n / 2;

    for (size_t i = 0; i + 1 < n; i += 2) {
        Object *name = OBJ_LIST_AT(args, i);
        VM_RECOVER_IF(vm, !OBJ_OFTYPE(name, TY_SYMBOL), vm->singletons._VALUE_EXCEPTION);

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

static void spk_if_form(VM *vm) {
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

static void spk_while_form(VM *vm) {
    Object *args = vm_peek(vm);
    VM_RECOVER_IF(vm, OBJ_LIST_SIZE(args) != 2, vm->singletons._VALUE_EXCEPTION);

    Object *condition = OBJ_LIST_AT(args, 0);
    Object *body = OBJ_LIST_AT(args, 1);

    // Nil stands in as the result until the body runs once, so a loop whose
    // condition is false from the start still leaves something behind.
    vm_build_nil(vm);

    vm_push(vm, condition);
    vm_eval_object(vm);
    bool truthy = vm_cast_to_bool(vm);

    while (truthy) {
        vm_pop(vm);
        vm_pop(vm);

        vm_push(vm, body);
        vm_eval_object(vm);

        vm_push(vm, condition);
        vm_eval_object(vm);
        truthy = vm_cast_to_bool(vm);
    }

    vm_pop(vm);
    vm_pop_prev(vm);
}

// (for value In list body...) or (for key value In list body...). The marker
// fixes where the names end: without it `(for x lst body1 body2)` reads equally
// well as one name over `lst` or as two names over `body1`, since the list
// position accepts a bare symbol. Same trick as Var in lambda.
static void spk_for_form(VM *vm) {
    Object *args = vm_peek(vm);
    size_t n = OBJ_LIST_SIZE(args);
    StringName marker = VM_SYM(vm, In);

    size_t names = 0;
    for (size_t i = 1; i <= 2 && i < n; i++) {
        Object *at = OBJ_LIST_AT(args, i);
        if (OBJ_OFTYPE(at, TY_SYMBOL) && OBJ_SYMBOL(at) == marker) {
            names = i;
            break;
        }
    }

    VM_RECOVER_IF(vm, names == 0, vm->singletons._VALUE_EXCEPTION);

    // The list expression sits right after the marker; everything past it is
    // the body, which may be empty.
    size_t list_at = names + 1;
    VM_RECOVER_IF(vm, list_at >= n, vm->singletons._VALUE_EXCEPTION);

    for (size_t i = 0; i < names; i++)
        VM_RECOVER_IF(vm, !OBJ_OFTYPE(OBJ_LIST_AT(args, i), TY_SYMBOL),
                      vm->singletons._VALUE_EXCEPTION);
    VM_RECOVER_IF(
        vm, names == 2 && OBJ_SYMBOL(OBJ_LIST_AT(args, 0)) == OBJ_SYMBOL(OBJ_LIST_AT(args, 1)),
        vm->singletons._VALUE_EXCEPTION);

    vm_push(vm, OBJ_LIST_AT(args, list_at));
    vm_eval_object(vm);
    VM_RECOVER_IF(vm, !OBJ_OFTYPE(vm_peek(vm), TY_LIST), vm->singletons._TYPE_EXCEPTION);

    Object *items = vm_peek(vm);

    // Nil stands in as the result until the body runs, so an empty list still
    // leaves something behind.
    vm_build_nil(vm);

    // The size is re-read every step rather than cached, so a body that shrinks
    // the list stops the loop instead of indexing past the end.
    for (size_t i = 0; i < OBJ_LIST_SIZE(items); i++) {
        // A scope per iteration, so a lambda made in the body captures that
        // step's binding rather than sharing one cell with every other step.
        vm_build_scope(vm);

        if (names == 2) {
            vm_build_integer(vm, (Integer)i);
            vm_scope_define(vm, OBJ_SYMBOL(OBJ_LIST_AT(args, 0)));
            vm_pop(vm);
        }

        vm_push(vm, OBJ_LIST_AT(items, i));
        vm_scope_define(vm, OBJ_SYMBOL(OBJ_LIST_AT(args, names - 1)));
        vm_pop(vm);

        for (size_t b = list_at + 1; b < n; b++) {
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

static void spk_lambda_form(VM *vm) {
    Object *args = vm_peek(vm);
    VM_RECOVER_IF(vm, OBJ_LIST_SIZE(args) != 2, vm->singletons._VALUE_EXCEPTION);

    Object *params = OBJ_LIST_AT(args, 0);
    Object *body = OBJ_LIST_AT(args, 1);

    VM_RECOVER_IF(vm, !OBJ_OFTYPE(params, TY_SYMBOL | TY_LIST), vm->singletons._VALUE_EXCEPTION);

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
    vm_pop_prev(vm);
}

// The kind to catch is evaluated before the frame is pushed, so an exception
// raised while producing it belongs to the enclosing handler - this try is not
// armed yet. The frame is then pushed with the stack already holding everything
// the handler reads, which lets the unwind discard the try scope and every
// intermediate value on its own.
static void spk_try_form(VM *vm) {
    Object *args = vm_peek(vm);
    size_t n = OBJ_LIST_SIZE(args);

    VM_RECOVER_IF(vm, n == 0, vm->singletons._VALUE_EXCEPTION);

    vm_push(vm, OBJ_LIST_AT(args, 0));
    vm_eval_object(vm);
    VM_RECOVER_IF(vm, !OBJ_OFTYPE(vm_peek(vm), TY_SYMBOL), vm->singletons._VALUE_EXCEPTION);

    jmp_buf env;
    vm_push_recovery(vm, &env);

    if (setjmp(env)) {
        // Unwound back to List, Symbol. This frame goes first: a kind we do not
        // catch has to reach the enclosing handler, and vm_recover always
        // raises into the topmost frame. Kinds compare by interned name, the
        // identity test for symbols here - two symbols spelled the same always
        // share one StringName, while object identity means nothing.
        vm_pop_recovery(vm);

        if (OBJ_SYMBOL(vm_peek(vm)) != OBJ_SYMBOL(vm->exception))
            vm_recover(vm, vm->exception);

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

static void spk_quote_form(VM *vm) {
    Object *args = vm_peek(vm);
    VM_RECOVER_IF(vm, OBJ_LIST_SIZE(args) != 1, vm->singletons._VALUE_EXCEPTION);

    Object *expr = OBJ_LIST_AT(args, 0);
    vm_pop(vm);
    vm_push(vm, expr);
}

static void spk_begin_form(VM *vm) {
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

static void spk_and_form(VM *vm) {
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

static void spk_or_form(VM *vm) {
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
