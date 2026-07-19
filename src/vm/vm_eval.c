#include "dynamic_array.h"
#include "object.h"
#include "special_forms.h"
#include "vm.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// List (args, unevaluated), Lambda -> Node (result)
static void eval_lambda_call(VM *vm) {
    assert(OFTYPE(vm_peek(vm), TY_LAMBDA));
    assert(OFTYPE(vm_prev(vm), TY_LIST));

    Object *lambda = vm_peek(vm);
    vm_swap(vm);

    Object *args = vm_peek(vm);
    size_t argc = LIST_SIZE(args);
    for (size_t i = 0; i < argc; i++) {
        vm_push(vm, LIST_AT(args, i));
        vm_eval_node(vm);
    }
    vm_pack_list(vm, argc);
    vm_pop_prev(vm);

    Object *evargs = vm_peek(vm);
    size_t pos_args = LAMBDA_POS_ARGS_N(lambda);

    bool bad_arity = LAMBDA_IS_VARIADIC(lambda) ? LIST_SIZE(evargs) < pos_args
                                                : LIST_SIZE(evargs) != pos_args;
    VM_RECOVER_IF(vm, bad_arity, vm->singletons._ARITY_EXCEPTION);

    vm_push_scope(vm, LAMBDA_SCOPE(lambda));
    vm_build_scope(vm);

    for (size_t i = 0; i < pos_args; i++) {
        vm_push(vm, LIST_AT(evargs, i));
        vm_scope_define(vm, da_at(LAMBDA_ARGS(lambda), i));
        vm_pop(vm);
    }

    if (LAMBDA_IS_VARIADIC(lambda)) {
        for (size_t i = pos_args; i < LIST_SIZE(evargs); i++)
            vm_push(vm, LIST_AT(evargs, i));
        vm_pack_list(vm, LIST_SIZE(evargs) - pos_args);
        vm_scope_define(vm, da_at_end(LAMBDA_ARGS(lambda), 0));
        vm_pop(vm);
    }

    vm_push(vm, LAMBDA_SUBEXPR(lambda));
    vm_eval_node(vm);

    vm_pop_scope(vm);
    vm_pop_scope(vm);

    vm_pop_prev(vm);
    vm_pop_prev(vm);
}

// List (args, unevaluated), Builtin -> Node (result)
static void eval_builtin_call(VM *vm) {
    assert(OFTYPE(vm_peek(vm), TY_BUILTIN));
    assert(OFTYPE(vm_prev(vm), TY_LIST));

    Object *builtin = vm_peek(vm);
    vm_pop(vm);

    Object *args = vm_peek(vm);
    size_t argc = LIST_SIZE(args);
    for (size_t i = 0; i < argc; i++) {
        vm_push(vm, LIST_AT(args, i));
        vm_eval_node(vm);
    }
    vm_pack_list(vm, argc);
    vm_pop_prev(vm);

    if (!BUILTIN_IS_VARIADIC(builtin)) {
        VM_RECOVER_IF(vm, argc != BUILTIN_ARGS_N(builtin), vm->singletons._ARITY_EXCEPTION);
        vm_unpack_list(vm);
    } else {
        VM_RECOVER_IF(vm, argc < BUILTIN_ARGS_N(builtin), vm->singletons._ARITY_EXCEPTION);
        vm_unpack_list(vm);
        vm_pack_list(vm, argc - BUILTIN_ARGS_N(builtin));
    }

    BUILTIN_FUNC(builtin)(vm);
}

// Node -> Bool
bool vm_cast_to_bool(VM *vm) {
    bool result = false;

    switch (vm_peek(vm)->kind) {
    case KIND_BOOL:
        result = BOOL(vm_peek(vm));
        break;
    case KIND_FLOAT:
        result = FLOAT(vm_peek(vm)) != 0.0;
        break;
    case KIND_INTEGER:
        result = INTEGER(vm_peek(vm)) != 0;
        break;
    case KIND_STRING:
        result = strlen(STRING(vm_peek(vm))) != 0;
        break;
    case KIND_LIST:
        result = LIST_SIZE(vm_peek(vm)) != 0;
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

// (Bool | Integer | Float), (Bool | Integer | Float) -> Node, Node
static ObjectKind vm_common_numeric(VM *vm) {
    assert(OFTYPE(vm_peek(vm), TY_NUMERIC));
    assert(OFTYPE(vm_prev(vm), TY_NUMERIC));

    if ((TYPEOF(vm_peek(vm)) | TYPEOF(vm_prev(vm))) & TY_FLOAT)
        return KIND_FLOAT;

    return KIND_INTEGER;
}

// (Bool | Integer | Float) -> Node
static void vm_cast_numeric(VM *vm, ObjectKind kind) {
    assert(OFTYPE(vm_peek(vm), TY_NUMERIC));

    if (OFTYPE(vm_peek(vm), TY_BOOL) && kind != KIND_BOOL) {
        vm_build_integer(vm, BOOL(vm_peek(vm)));
        vm_pop_prev(vm);
    }

    if (OFTYPE(vm_peek(vm), TY_INTEGER) && kind == KIND_FLOAT) {
        vm_build_float(vm, INTEGER(vm_peek(vm)));
        vm_pop_prev(vm);
    }
}

// (Bool | Integer | Float), (Bool | Integer | Float) -> Node, Node
ObjectKind vm_to_common_numeric(VM *vm) {
    assert(OFTYPE(vm_peek(vm), TY_NUMERIC));
    assert(OFTYPE(vm_prev(vm), TY_NUMERIC));

    ObjectKind common = vm_common_numeric(vm);

    vm_cast_numeric(vm, common);
    vm_swap(vm);
    vm_cast_numeric(vm, common);
    vm_swap(vm);

    return common;
}

// List -> Node
static void vm_eval_list(VM *vm) {
    ASSERT_KIND(vm, KIND_LIST);

    Object *call = vm_peek(vm);
    size_t size = LIST_SIZE(call);

    if (size == 0)
        return;

    Object *head = LIST_AT(call, 0);

    for (size_t i = 1; i < size; i++)
        vm_push(vm, LIST_AT(call, i));
    vm_pack_list(vm, size - 1);

    if (OFTYPE(head, TY_SYMBOL) && try_dispatch_special_form(vm, SYMBOL(head))) {
        vm_pop_prev(vm);
        return;
    }

    vm_push(vm, head);
    vm_eval_node(vm);
    vm_rot(vm);
    vm_pop(vm);

    switch (vm_peek(vm)->kind) {
    case KIND_LAMBDA:
        eval_lambda_call(vm);
        break;

    case KIND_BUILTIN:
        eval_builtin_call(vm);
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

// Symbol -> Node
static void vm_eval_symbol(VM *vm) {
    ASSERT_KIND(vm, KIND_SYMBOL);

    StringName name = SYMBOL(vm_peek(vm));
    vm_pop(vm);

    if (name == vm->si->prebuilt._Nil)
        vm_build_nil(vm);
    else if (name == vm->si->prebuilt._True)
        vm_build_bool(vm, true);
    else if (name == vm->si->prebuilt._False)
        vm_build_bool(vm, false);
    else if ('A' <= name[0] && name[0] <= 'Z')
        vm_build_symbol(vm, name);
    else
        vm_scope_get(vm, name);
}

// Node -> Node
void vm_eval_node(VM *vm) {
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
