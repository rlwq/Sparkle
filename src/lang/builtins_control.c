#include "builtins.h"
#include "object.h"
#include "vm.h"

#include <assert.h>

// Value -> Value
static void spk_eval(VM *vm) {
    vm_eval_object(vm);
}

// Symbol (kind), Value... -> (does not return)
// A single trailing value pairs into an Exception carrying it; with none, the
// bare kind Symbol is raised, so a value-less throw stays a plain kind.
static void spk_throw(VM *vm) {
    assert(OBJ_OFTYPE(vm_peek(vm), TY_LIST));
    Object *rest = vm_peek(vm);
    VM_RECOVER_IF(vm, !OBJ_OFTYPE(vm_prev(vm), TY_SYMBOL), vm->singletons._TYPE_EXCEPTION);
    VM_RECOVER_IF(vm, OBJ_LIST_SIZE(rest) > 1, vm->singletons._VALUE_EXCEPTION);

    if (OBJ_LIST_SIZE(rest) == 0) {
        vm_pop(vm);
        vm_recover(vm, vm_peek(vm));
    }

    // kind (vm_prev) and value (inside rest) stay rooted on the stack across the
    // build; the fresh Exception is then what unwinds.
    vm_build_exception(vm, vm_prev(vm), OBJ_LIST_AT(rest, 0));
    vm_recover(vm, vm_peek(vm));
}

// Exception -> Symbol (kind)
static void spk_exc_kind(VM *vm) {
    vm_expect(vm, TY_EXCEPTION);
    Object *kind = OBJ_EXCEPTION_KIND(vm_peek(vm));
    vm_pop(vm);
    vm_push(vm, kind);
}

// Exception -> Value
static void spk_exc_value(VM *vm) {
    vm_expect(vm, TY_EXCEPTION);
    Object *value = OBJ_EXCEPTION_VALUE(vm_peek(vm));
    vm_pop(vm);
    vm_push(vm, value);
}

// Callable, List (args) -> Value
static void spk_apply(VM *vm) {
    vm_expect(vm, TY_LIST);
    VM_RECOVER_IF(vm, !OBJ_OFTYPE(vm_prev(vm), TY_CALLABLE), vm->singletons._UNCALLABLE_EXCEPTION);

    vm_swap(vm);
    vm_call(vm);
}

DEFINE_MODULE(CONTROL) = {
    {"eval", spk_eval, 1, false},           {"throw", spk_throw, 1, true},
    {"apply", spk_apply, 2, false},         {"exc-kind", spk_exc_kind, 1, false},
    {"exc-value", spk_exc_value, 1, false},
};

DEFINE_MODULE_SIZE(CONTROL);
