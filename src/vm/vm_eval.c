#include "dynamic_array.h"
#include "object.h"
#include "special_forms.h"
#include "vm.h"
#include <assert.h>
#include <stdbool.h>

// Cons (args), Node (lambda) -> Node (result)
void eval_lambda_call(VM *vm) {

    Object *lambda = vm_peek(vm);
    vm_swap(vm);

    vm_push_scope(vm, LAMBDA_SCOPE(lambda));
    vm_build_scope(vm);

    for (size_t i = 0; i < LAMBDA_POS_ARGS_N(lambda); i++) {
        if (vm_peek(vm)->kind != KIND_CONS)
            vm_recover(vm, WRONG_ARITY);
        vm_unpack_cons(vm);
        vm_scope_define(vm, da_at(LAMBDA_ARGS(lambda), i));
        vm_pop(vm);
    }

    if (!LAMBDA_IS_VARIADIC(lambda) && vm_peek(vm)->kind != KIND_NIL)
        vm_recover(vm, WRONG_ARITY);

    if (LAMBDA_IS_VARIADIC(lambda))
        vm_scope_define(vm, da_at_end(LAMBDA_ARGS(lambda), 0));
    vm_pop(vm);

    vm_push(vm, LAMBDA_SUBEXPR(lambda));
    vm_eval_node(vm);
    vm_pop_prev(vm);

    vm_pop_scope(vm);
    vm_pop_scope(vm);
}

// Node (cons) -> Node (cons)
size_t vm_eval_list(VM *vm) {
    size_t length = 0;
    while (OFTYPE(vm_peek(vm), TY_CONS)) {
        vm_unpack_cons(vm);
        vm_eval_node(vm);
        vm_swap(vm);
        length++;
    }

    ASSERT_KIND(vm, KIND_NIL);

    for (size_t i = 0; i < length; i++) {
        vm_swap(vm);
        vm_pack_cons(vm);
    }

    return length;
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
        assert(0);
        break;
    case KIND_CONS:
    case KIND_SYMBOL:
    case KIND_BUILTIN:
    case KIND_LAMBDA:
    case KIND_EXCEPTION:
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

void vm_unpack_list_n(VM *vm, size_t n) {
    for (; n > 0; n--) {
        ASSERT_KIND(vm, KIND_CONS);
        vm_unpack_cons(vm);
        vm_swap(vm);
    }
}

// (Bool | Integer | Float), (Bool | Integer | Float) -> Node, Node
ObjectKind vm_common_numeric(VM *vm) {
    assert(OFTYPE(vm_peek(vm), TY_NUMERIC));
    assert(OFTYPE(vm_prev(vm), TY_NUMERIC));

    if ((TYPEOF(vm_peek(vm)) | TYPEOF(vm_prev(vm))) & TY_FLOAT)
        return KIND_FLOAT;

    return KIND_INTEGER;
}

// (Bool | Integer | Float) -> Node
void vm_cast_numeric(VM *vm, ObjectKind kind) {
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
    ObjectKind common = vm_common_numeric(vm);

    vm_cast_numeric(vm, common);
    vm_swap(vm);
    vm_cast_numeric(vm, common);
    vm_swap(vm);

    return common;
}

// Cons -> Node
void vm_eval_cons(VM *vm) {
    ASSERT_KIND(vm, KIND_CONS);

    vm_unpack_cons(vm);

    bool is_special_form = try_dispatch_special_form(vm);
    if (is_special_form)
        return;

    vm_eval_node(vm);

    switch (vm_peek(vm)->kind) {
    case KIND_LAMBDA:
        vm_swap(vm);
        vm_eval_list(vm);

        vm_dup_prev(vm);
        eval_lambda_call(vm);
        break;

    case KIND_BUILTIN: {
        Object *builtin = vm_peek(vm);
        vm_swap(vm);
        size_t args_count = vm_eval_list(vm);

        if (!BUILTIN_IS_VARIADIC(builtin)) {
            VM_RECOVER_IF(vm, args_count != BUILTIN_ARGS_N(builtin), WRONG_ARITY);
            vm_unpack_list(vm);
        }

        if (BUILTIN_IS_VARIADIC(builtin)) {
            VM_RECOVER_IF(vm, args_count < BUILTIN_ARGS_N(builtin), WRONG_ARITY);
            vm_unpack_list_n(vm, BUILTIN_ARGS_N(builtin));
        }
        BUILTIN_FUNC(builtin)(vm);
        break;
    }

    case KIND_CONS:
    case KIND_BOOL:
    case KIND_FLOAT:
    case KIND_SYMBOL:
    case KIND_INTEGER:
    case KIND_STRING:
    case KIND_EXCEPTION:
    case KIND_NIL:
        vm_recover(vm, UNCALLABLE_CALL);
        break;
    }

    vm_pop_prev(vm);
}

// Symbol -> Node
void vm_eval_symbol(VM *vm) {
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
    case KIND_EXCEPTION:
        // Do nothing
        break;

    case KIND_SYMBOL:
        vm_eval_symbol(vm);
        break;

    case KIND_CONS:
        vm_eval_cons(vm);
        break;
    }
}
