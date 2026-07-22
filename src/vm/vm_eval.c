#include "dynamic_array.h"
#include "object.h"
#include "vm.h"

#include <assert.h>
#include <stdbool.h>

// List (args) -> List (args evaluated)
static void vm_eval_args(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));

    Object *args = vm_peek(vm);
    size_t argc = OBJ_LIST_SIZE(args);
    for (size_t i = 0; i < argc; i++) {
        vm_push(vm, OBJ_LIST_AT(args, i));
        vm_eval_object(vm);
    }
    vm_pack_list(vm, argc);
    vm_pop_prev(vm);
}

// List (args), Lambda -> Value
static void vm_call_lambda(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LAMBDA));
    assert(OBJ_OFTYPE(vm_prev(vm), TY_LIST));

    Object *lambda = vm_peek(vm);
    Object *evargs = vm_prev(vm);
    size_t pos_args = OBJ_LAMBDA_POS_ARGS_N(lambda);

    bool bad_arity = OBJ_LAMBDA_IS_VARIADIC(lambda) ? OBJ_LIST_SIZE(evargs) < pos_args
                                                    : OBJ_LIST_SIZE(evargs) != pos_args;
    VM_RECOVER_IF(vm, bad_arity, vm->singletons._ARITY_EXCEPTION);

    vm_push_scope(vm, OBJ_LAMBDA_SCOPE(lambda));
    vm_build_scope(vm);

    for (size_t i = 0; i < pos_args; i++) {
        vm_push(vm, OBJ_LIST_AT(evargs, i));
        vm_scope_define(vm, da_at(OBJ_LAMBDA_ARGS(lambda), i));
        vm_pop(vm);
    }

    if (OBJ_LAMBDA_IS_VARIADIC(lambda)) {
        for (size_t i = pos_args; i < OBJ_LIST_SIZE(evargs); i++)
            vm_push(vm, OBJ_LIST_AT(evargs, i));
        vm_pack_list(vm, OBJ_LIST_SIZE(evargs) - pos_args);
        vm_scope_define(vm, da_at_end(OBJ_LAMBDA_ARGS(lambda), 0));
        vm_pop(vm);
    }

    vm_push(vm, OBJ_LAMBDA_SUBEXPR(lambda));
    vm_eval_object(vm);

    vm_pop_scope(vm);
    vm_pop_scope(vm);

    vm_pop_prev(vm);
    vm_pop_prev(vm);
}

// List (args), Builtin -> Value
static void vm_call_builtin(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_BUILTIN));
    assert(OBJ_OFTYPE(vm_prev(vm), TY_LIST));

    // The builtin object survives the collection points below unrooted: every
    // builtin is permanently reachable through the global scope it was
    // registered in.
    Object *builtin = vm_peek(vm);
    vm_pop(vm);

    size_t argc = OBJ_LIST_SIZE(vm_peek(vm));

    if (!OBJ_BUILTIN_IS_VARIADIC(builtin)) {
        VM_RECOVER_IF(vm, argc != OBJ_BUILTIN_ARGS_N(builtin), vm->singletons._ARITY_EXCEPTION);
        vm_unpack_list(vm);
    } else {
        VM_RECOVER_IF(vm, argc < OBJ_BUILTIN_ARGS_N(builtin), vm->singletons._ARITY_EXCEPTION);
        vm_unpack_list(vm);
        vm_pack_list(vm, argc - OBJ_BUILTIN_ARGS_N(builtin));
    }

    OBJ_BUILTIN_FUNC(builtin)(vm);
}

// List (args), Callable -> Value
void vm_call(VM *vm) {
    switch (vm_peek(vm)->kind) {
    case KIND_LAMBDA:
        vm_call_lambda(vm);
        break;

    case KIND_BUILTIN:
        vm_call_builtin(vm);
        break;

    case KIND_LIST:
    case KIND_BOOL:
    case KIND_FLOAT:
    case KIND_SYMBOL:
    case KIND_INTEGER:
    case KIND_STRING:
    case KIND_NIL:
        vm_recover(vm, vm->singletons._UNCALLABLE_EXCEPTION);
        break;
    }
}

// List -> Value
static void vm_eval_list(VM *vm) {
    ASSERT_KIND(vm, KIND_LIST);

    Object *call = vm_peek(vm);
    size_t size = OBJ_LIST_SIZE(call);

    if (size == 0)
        return;

    Object *head = OBJ_LIST_AT(call, 0);

    for (size_t i = 1; i < size; i++)
        vm_push(vm, OBJ_LIST_AT(call, i));
    vm_pack_list(vm, size - 1);

    if (OBJ_OFTYPE(head, TY_SYMBOL) && vm_try_special_form(vm, OBJ_SYMBOL(head))) {
        vm_pop_prev(vm);
        return;
    }

    vm_push(vm, head);
    vm_eval_object(vm);
    vm_rot(vm);
    vm_pop(vm);

    vm_swap(vm);
    vm_eval_args(vm);
    vm_swap(vm);
    vm_call(vm);
}

// Symbol -> Value
static void vm_eval_symbol(VM *vm) {
    ASSERT_KIND(vm, KIND_SYMBOL);

    StringName name = OBJ_SYMBOL(vm_peek(vm));
    vm_pop(vm);

    if (name == VM_SYM(vm, Nil))
        vm_build_nil(vm);
    else if (name == VM_SYM(vm, True))
        vm_build_bool(vm, true);
    else if (name == VM_SYM(vm, False))
        vm_build_bool(vm, false);
    else if ('A' <= name[0] && name[0] <= 'Z')
        vm_build_symbol(vm, name);
    else
        vm_scope_get(vm, name);
}

// Value -> Value
void vm_eval_object(VM *vm) {
    switch (vm_peek(vm)->kind) {
    case KIND_NIL:
    case KIND_INTEGER:
    case KIND_FLOAT:
    case KIND_BOOL:
    case KIND_STRING:
    case KIND_BUILTIN:
    case KIND_LAMBDA:
        break;

    case KIND_SYMBOL:
        vm_eval_symbol(vm);
        break;

    case KIND_LIST:
        vm_eval_list(vm);
        break;
    }
}
