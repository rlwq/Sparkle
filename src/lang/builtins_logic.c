#include "builtins.h"
#include "object.h"
#include "vm.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

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

// Object, Object -> Object
static void spk_eq(VM *vm) {
    bool is_equal = false;

    if (OBJ_OFTYPE(vm_peek(vm), TY_NUMERIC) && OBJ_OFTYPE(vm_prev(vm), TY_NUMERIC))
        vm_to_common_numeric(vm);

    if (vm_peek(vm)->kind != vm_prev(vm)->kind) {
        is_equal = false;
        goto end;
    }

    switch (vm_peek(vm)->kind) {
    case KIND_NIL:
        is_equal = true;
        break;
    case KIND_SYMBOL:
        is_equal = OBJ_SYMBOL(vm_peek(vm)) == OBJ_SYMBOL(vm_prev(vm));
        break;
    case KIND_INTEGER:
        is_equal = OBJ_INTEGER(vm_peek(vm)) == OBJ_INTEGER(vm_prev(vm));
        break;
    case KIND_FLOAT:
        is_equal = OBJ_FLOAT(vm_peek(vm)) == OBJ_FLOAT(vm_prev(vm));
        break;
    case KIND_BOOL:
        is_equal = OBJ_BOOL(vm_peek(vm)) == OBJ_BOOL(vm_prev(vm));
        break;
    case KIND_STRING:
        is_equal = OBJ_STRING_SIZE(vm_peek(vm)) == OBJ_STRING_SIZE(vm_prev(vm)) &&
                   memcmp(OBJ_STRING_DATA(vm_peek(vm)), OBJ_STRING_DATA(vm_prev(vm)),
                          OBJ_STRING_SIZE(vm_peek(vm))) == 0;
        break;
    case KIND_LIST:
    case KIND_LAMBDA:
    case KIND_BUILTIN:
        is_equal = vm_peek(vm) == vm_prev(vm);
        break;
    }

end:
    vm_pop_n(vm, 2);
    vm_build_bool(vm, is_equal);
}

static void spk_ne(VM *vm) {
    spk_eq(vm);
    vm_build_bool(vm, !OBJ_BOOL(vm_peek(vm)));
    vm_pop_prev(vm);
}

NUMERIC_ORDER_BUILTIN(spk_gt, >)
NUMERIC_ORDER_BUILTIN(spk_ge, >=)
NUMERIC_ORDER_BUILTIN(spk_lt, <)
NUMERIC_ORDER_BUILTIN(spk_le, <=)

static void spk_cast_to_bool(VM *vm) {
    vm_cast_to_bool(vm);
}

static void spk_logical_and(VM *vm) {
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

static void spk_logical_or(VM *vm) {
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
