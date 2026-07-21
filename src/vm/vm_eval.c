#include "dynamic_array.h"
#include "object.h"
#include "vm.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// List (args, unevaluated) -> List (args, evaluated)
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

// List (args, evaluated), Lambda -> Object (result)
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

// List (args, evaluated), Builtin -> Object (result)
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

// List (args, evaluated), Callable -> Object (result)
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

// Object -> Bool
bool vm_cast_to_bool(VM *vm) {
    bool result = false;

    switch (vm_peek(vm)->kind) {
    case KIND_BOOL:
        result = OBJ_BOOL(vm_peek(vm));
        break;
    case KIND_FLOAT:
        result = OBJ_FLOAT(vm_peek(vm)) != 0.0;
        break;
    case KIND_INTEGER:
        result = OBJ_INTEGER(vm_peek(vm)) != 0;
        break;
    case KIND_STRING:
        result = OBJ_STRING_SIZE(vm_peek(vm)) != 0;
        break;
    case KIND_LIST:
        result = OBJ_LIST_SIZE(vm_peek(vm)) != 0;
        break;
    case KIND_SYMBOL:
    case KIND_BUILTIN:
    case KIND_LAMBDA:
        result = true;
        break;
    case KIND_NIL:
        result = false;
        break;
    }

    vm_pop(vm);
    vm_build_bool(vm, result);
    return result;
}

// (Bool | Integer | Float), (Bool | Integer | Float) -> Object, Object
static ObjectKind vm_common_numeric(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_NUMERIC));
    assert(OBJ_OFTYPE(vm_prev(vm), TY_NUMERIC));

    if ((OBJ_TYPEOF(vm_peek(vm)) | OBJ_TYPEOF(vm_prev(vm))) & TY_FLOAT)
        return KIND_FLOAT;

    return KIND_INTEGER;
}

// (Bool | Integer | Float) -> Object
static void vm_cast_numeric(VM *vm, ObjectKind kind) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_NUMERIC));

    if (OBJ_OFTYPE(vm_peek(vm), TY_BOOL) && kind != KIND_BOOL) {
        vm_build_integer(vm, OBJ_BOOL(vm_peek(vm)));
        vm_pop_prev(vm);
    }

    if (OBJ_OFTYPE(vm_peek(vm), TY_INTEGER) && kind == KIND_FLOAT) {
        vm_build_float(vm, OBJ_INTEGER(vm_peek(vm)));
        vm_pop_prev(vm);
    }
}

// (Bool | Integer | Float), (Bool | Integer | Float) -> Object, Object
ObjectKind vm_to_common_numeric(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_NUMERIC));
    assert(OBJ_OFTYPE(vm_prev(vm), TY_NUMERIC));

    ObjectKind common = vm_common_numeric(vm);

    vm_cast_numeric(vm, common);
    vm_swap(vm);
    vm_cast_numeric(vm, common);
    vm_swap(vm);

    return common;
}

// List -> Object
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

// Symbol -> Object
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

// Object -> Object
void vm_eval_object(VM *vm) {
    switch (vm_peek(vm)->kind) {
    case KIND_NIL:
    case KIND_INTEGER:
    case KIND_FLOAT:
    case KIND_BOOL:
    case KIND_STRING:
    case KIND_BUILTIN:
    case KIND_LAMBDA:
        // Do nothing
        break;

    case KIND_SYMBOL:
        vm_eval_symbol(vm);
        break;

    case KIND_LIST:
        vm_eval_list(vm);
        break;
    }
}
