#include <stdbool.h>

#include "builtins.h"
#include "object.h"
#include "vm.h"

// ... -> List
void rkl_list(VM *vm) {
    /* The args. list is already on the stack. Does nothing */
    (void)vm;
}

// List -> Integer
void rkl_len(VM *vm) {
    vm_expect(vm, TY_LIST);

    Integer n = (Integer)OBJ_LIST_SIZE(vm_peek(vm));
    vm_pop(vm);
    vm_build_integer(vm, n);
}

// List, Integer -> Value
void rkl_get(VM *vm) {
    vm_expect2(vm, TY_LIST, TY_INTEGER);

    Object *l = vm_prev(vm);
    Integer idx = OBJ_INTEGER(vm_peek(vm));

    VM_RECOVER_IF(vm, idx < 0 || (size_t)idx >= OBJ_LIST_SIZE(l), vm->singletons._VALUE_EXCEPTION);

    Object *e = OBJ_LIST_AT(l, idx);
    vm_pop_n(vm, 2);
    vm_push(vm, e);
}

// List, Integer, Value -> List
void rkl_put(VM *vm) {
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
void rkl_push(VM *vm) {
    vm_expect2(vm, TY_LIST, TY_ANY);

    Object *x = vm_peek(vm);
    Object *l = vm_prev(vm);
    da_push(OBJ_LIST_ITEMS(l), x);
    vm_pop(vm);
}

// List -> Value
void rkl_pop(VM *vm) {
    vm_expect(vm, TY_LIST);

    Object *l = vm_peek(vm);
    VM_RECOVER_IF(vm, OBJ_LIST_SIZE(l) == 0, vm->singletons._VALUE_EXCEPTION);

    Object *e = OBJ_LIST_AT(l, OBJ_LIST_SIZE(l) - 1);
    da_pop(OBJ_LIST_ITEMS(l));
    vm_pop(vm);
    vm_push(vm, e);
}

// List, List -> List
void rkl_append(VM *vm) {
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
void rkl_map(VM *vm) {
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
void rkl_filter(VM *vm) {
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
    {"list", rkl_list, 0, true},      {"len", rkl_len, 1, false},
    {"get", rkl_get, 2, false},       {"put", rkl_put, 3, false},
    {"push", rkl_push, 2, false},     {"pop", rkl_pop, 1, false},
    {"append", rkl_append, 2, false}, {"map", rkl_map, 2, false},
    {"filter", rkl_filter, 2, false},
};

DEFINE_MODULE_SIZE(LIST);
