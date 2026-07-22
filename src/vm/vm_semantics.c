// The value-semantics primitives named in vm.h: truthiness, numeric coercion,
// equality. The lang/ layer borrows these; the rules are defined only here.
#include "object.h"
#include "vm.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

// Value -> Bool
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

// Numeric, Numeric -> KIND_INTEGER | KIND_FLOAT
static ObjectKind vm_common_numeric(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_NUMERIC));
    assert(OBJ_OFTYPE(vm_prev(vm), TY_NUMERIC));

    if ((OBJ_TYPEOF(vm_peek(vm)) | OBJ_TYPEOF(vm_prev(vm))) & TY_FLOAT)
        return KIND_FLOAT;

    return KIND_INTEGER;
}

// Numeric -> Numeric
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

// Numeric, Numeric -> Numeric, Numeric
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

// Value, Value -> Bool
bool vm_eq(VM *vm) {
    assert(vm->value_stack.size >= 2);
    bool is_equal = false;

    // A value is equal to itself, taken first so (= x x) on a self-referential
    // list terminates before the structural walk below recurses forever. The
    // one exception is a Float NaN, which IEEE 754 makes equal to nothing,
    // itself included: it is let through to the numeric comparison that says so.
    if (vm_peek(vm) == vm_prev(vm) &&
        !(vm_peek(vm)->kind == KIND_FLOAT && isnan(OBJ_FLOAT(vm_peek(vm))))) {
        is_equal = true;
        goto end;
    }

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
    case KIND_LIST: {
        // Structural: same length, each element equal by vm_eq itself. The two
        // lists stay rooted below the pushed pair while every element collects.
        Object *l1 = vm_prev(vm);
        Object *l2 = vm_peek(vm);
        size_t size = OBJ_LIST_SIZE(l1);
        is_equal = size == OBJ_LIST_SIZE(l2);
        for (size_t i = 0; is_equal && i < size; i++) {
            vm_push(vm, OBJ_LIST_AT(l1, i));
            vm_push(vm, OBJ_LIST_AT(l2, i));
            is_equal = vm_eq(vm);
            vm_pop(vm);
        }
        break;
    }
    case KIND_LAMBDA:
    case KIND_BUILTIN:
        is_equal = vm_peek(vm) == vm_prev(vm);
        break;
    }

end:
    vm_pop_n(vm, 2);
    vm_build_bool(vm, is_equal);
    return is_equal;
}
