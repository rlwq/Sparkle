#include "builtins.h"
#include "object.h"
#include "vm.h"

#include <assert.h>
#include <stdbool.h>

// Numeric, Numeric -> Bool
#define NUMERIC_ORDER_BUILTIN(func_name_, operator_)                                               \
    static void func_name_(VM *vm) {                                                               \
        vm_expect2(vm, TY_NUMERIC, TY_NUMERIC);                                                    \
        vm_to_common_numeric(vm);                                                                  \
        switch (OBJ_TYPEOF(vm_peek(vm))) {                                                         \
        case TY_BOOL:                                                                              \
            vm_build_bool(vm, OBJ_BOOL(vm_prev(vm)) operator_ OBJ_BOOL(vm_peek(vm)));              \
            break;                                                                                 \
        case TY_INTEGER:                                                                           \
            vm_build_bool(vm, OBJ_INTEGER(vm_prev(vm)) operator_ OBJ_INTEGER(vm_peek(vm)));        \
            break;                                                                                 \
        case TY_FLOAT:                                                                             \
            vm_build_bool(vm, OBJ_FLOAT(vm_prev(vm)) operator_ OBJ_FLOAT(vm_peek(vm)));            \
            break;                                                                                 \
        default:                                                                                   \
            assert(false && "UNREACHABLE");                                                        \
        }                                                                                          \
        vm_pop_prev_n(vm, 2);                                                                      \
    }

// Value, Value -> Bool
static void spk_eq(VM *vm) {
    assert(vm->value_stack.size >= 2);
    vm_eq(vm);
}

// Value, Value -> Bool
static void spk_ne(VM *vm) {
    assert(vm->value_stack.size >= 2);
    vm_build_bool(vm, !vm_eq(vm));
    vm_pop_prev(vm);
}

NUMERIC_ORDER_BUILTIN(spk_gt, >)
NUMERIC_ORDER_BUILTIN(spk_ge, >=)
NUMERIC_ORDER_BUILTIN(spk_lt, <)
NUMERIC_ORDER_BUILTIN(spk_le, <=)

// Value -> Bool
static void spk_cast_to_bool(VM *vm) {
    vm_cast_to_bool(vm);
}

// List of Value -> Bool   (variadic: args arrive packed)
static void spk_logical_and(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));
    bool result = true;

    Object *args_ = vm_peek(vm);
    OBJ_LIST_FOREACH(curr, args_)
        vm_push(vm, curr);
        vm_cast_to_bool(vm);
        result = result && OBJ_BOOL(vm_peek(vm));
        vm_pop(vm);
    OBJ_END_LIST_FOREACH

    vm_pop(vm);
    vm_build_bool(vm, result);
}

// List of Value -> Bool   (variadic: args arrive packed)
static void spk_logical_or(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));
    bool result = false;

    Object *args_ = vm_peek(vm);
    OBJ_LIST_FOREACH(curr, args_)
        vm_push(vm, curr);
        vm_cast_to_bool(vm);
        result = result || OBJ_BOOL(vm_peek(vm));
        vm_pop(vm);
    OBJ_END_LIST_FOREACH

    vm_pop(vm);
    vm_build_bool(vm, result);
}

// Value -> Bool
static void spk_logical_not(VM *vm) {
    vm_cast_to_bool(vm);
    vm_build_bool(vm, !OBJ_BOOL(vm_peek(vm)));
    vm_pop_prev(vm);
}

DEFINE_MODULE(LOGIC) = {
    {"=", spk_eq, 2, false},           {"!=", spk_ne, 2, false},
    {">", spk_gt, 2, false},           {">=", spk_ge, 2, false},
    {"<", spk_lt, 2, false},           {"<=", spk_le, 2, false},
    {"?", spk_cast_to_bool, 1, false}, {"not", spk_logical_not, 1, false},
    {"&&", spk_logical_and, 0, true},  {"||", spk_logical_or, 0, true},
};

DEFINE_MODULE_SIZE(LOGIC);
