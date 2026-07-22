#include "builtins.h"
#include "object.h"
#include "vm.h"

#include <assert.h>
#include <stdbool.h>

// Value... -> List   (variadic: args arrive packed)
static void spk_list(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));
    (void)vm;
}

// List -> Integer
static void spk_len(VM *vm) {
    vm_expect(vm, TY_LIST);

    Integer n = (Integer)OBJ_LIST_SIZE(vm_peek(vm));
    vm_pop(vm);
    vm_build_integer(vm, n);
}

// Value, Integer... -> Value   (variadic: indices arrive packed)
static void spk_get(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));

    // obj and the index list stay rooted on the stack; the descent allocates
    // nothing, so the walking pointer stays valid the whole way down.
    Object *indices = vm_peek(vm);
    Object *obj = vm_prev(vm);

    for (size_t k = 0; k < OBJ_LIST_SIZE(indices); k++) {
        Object *iobj = OBJ_LIST_AT(indices, k);
        VM_RECOVER_IF(vm, !OBJ_OFTYPE(obj, TY_LIST), vm->singletons._TYPE_EXCEPTION);
        VM_RECOVER_IF(vm, !OBJ_OFTYPE(iobj, TY_INTEGER), vm->singletons._TYPE_EXCEPTION);

        Integer idx = OBJ_INTEGER(iobj);
        VM_RECOVER_IF(vm, idx < 0 || (size_t)idx >= OBJ_LIST_SIZE(obj),
                      vm->singletons._VALUE_EXCEPTION);

        obj = OBJ_LIST_AT(obj, idx);
    }

    vm_pop_n(vm, 2);
    vm_push(vm, obj);
}

// List, Integer (index), Value -> List
static void spk_put(VM *vm) {
    Object *l = vm_peek_at(vm, 2);
    Object *iobj = vm_prev(vm);
    Object *x = vm_peek(vm);

    VM_RECOVER_IF(vm, !OBJ_OFTYPE(l, TY_LIST), vm->singletons._TYPE_EXCEPTION);
    VM_RECOVER_IF(vm, !OBJ_OFTYPE(iobj, TY_INTEGER), vm->singletons._TYPE_EXCEPTION);

    Integer idx = OBJ_INTEGER(iobj);
    VM_RECOVER_IF(vm, idx < 0 || (size_t)idx >= OBJ_LIST_SIZE(l), vm->singletons._VALUE_EXCEPTION);

    OBJ_LIST_AT(l, idx) = x;
    vm_pop_n(vm, 2);
}

// List, Value -> List
static void spk_push(VM *vm) {
    vm_expect2(vm, TY_LIST, TY_ANY);

    Object *x = vm_peek(vm);
    Object *l = vm_prev(vm);
    da_push(OBJ_LIST_ITEMS(l), x);
    vm_pop(vm);
}

// List -> Value
static void spk_pop(VM *vm) {
    vm_expect(vm, TY_LIST);

    Object *l = vm_peek(vm);
    VM_RECOVER_IF(vm, OBJ_LIST_SIZE(l) == 0, vm->singletons._VALUE_EXCEPTION);

    Object *e = OBJ_LIST_AT(l, OBJ_LIST_SIZE(l) - 1);
    da_pop(OBJ_LIST_ITEMS(l));
    vm_pop(vm);
    vm_push(vm, e);
}

// List, List -> List
static void spk_append(VM *vm) {
    vm_expect2(vm, TY_LIST, TY_LIST);

    vm_build_list(vm);

    Object *new_ = vm_peek(vm);
    Object *l1 = vm_peek_at(vm, 2);
    Object *l2 = vm_peek_at(vm, 1);

    for (size_t i = 0; i < OBJ_LIST_SIZE(l1); i++)
        da_push(OBJ_LIST_ITEMS(new_), OBJ_LIST_AT(l1, i));
    for (size_t i = 0; i < OBJ_LIST_SIZE(l2); i++)
        da_push(OBJ_LIST_ITEMS(new_), OBJ_LIST_AT(l2, i));

    vm_pop_prev_n(vm, 2);
}

// Callable, List -> List
static void spk_map(VM *vm) {
    vm_expect2(vm, TY_CALLABLE, TY_LIST);

    Object *func = vm_prev(vm);
    Object *l = vm_peek(vm);
    size_t n = OBJ_LIST_SIZE(l);

    for (size_t i = 0; i < n; i++) {
        vm_push(vm, OBJ_LIST_AT(l, i));
        vm_pack_list(vm, 1);
        vm_push(vm, func);
        vm_call(vm);
    }

    vm_pack_list(vm, n);
    vm_pop_prev_n(vm, 2);
}

// Callable, List -> List
static void spk_filter(VM *vm) {
    vm_expect2(vm, TY_CALLABLE, TY_LIST);

    Object *func = vm_prev(vm);
    Object *l = vm_peek(vm);
    size_t n = OBJ_LIST_SIZE(l);
    size_t kept = 0;

    for (size_t i = 0; i < n; i++) {
        vm_push(vm, OBJ_LIST_AT(l, i));
        vm_pack_list(vm, 1);
        vm_push(vm, func);
        vm_call(vm);
        vm_cast_to_bool(vm);
        bool keep = OBJ_BOOL(vm_peek(vm));
        vm_pop(vm);
        if (keep) {
            vm_push(vm, OBJ_LIST_AT(l, i));
            kept++;
        }
    }

    vm_pack_list(vm, kept);
    vm_pop_prev_n(vm, 2);
}

DEFINE_MODULE(LIST) = {
    {"list", spk_list, 0, true},      {"len", spk_len, 1, false},
    {"get", spk_get, 1, true},        {"put", spk_put, 3, false},
    {"push", spk_push, 2, false},     {"pop", spk_pop, 1, false},
    {"append", spk_append, 2, false}, {"map", spk_map, 2, false},
    {"filter", spk_filter, 2, false},
};

DEFINE_MODULE_SIZE(LIST);
